#include "common.h"
#include "uart.h"
#include "print.h"

uint16_t global_cnt;
uint16_t global_value;

#ifdef _DEBUG_MODE
void DoPrint( const char CODE_P *fmt, va_list ap )
{
	char  ch;
	char  i;
	long  value;
	bit   fl_zero;
	bit   fl_num;
	uint8_t  fl_len;
	uint8_t  cnt;
  char *ptr;
	uint32_t mask=1;
  uint8_t	Hex[17] = "0123456789ABCDEF";

	while(1) {
		
		//----- Find Formatter % -----

		switch( ch = *fmt++ ) {
			case 0:		return;
			case '%':	if( *fmt != '%' ) break;
						fmt++;
			default:	_outchar( ch );
						continue;
		}

		//----- Get Count -------------
		
		fl_zero = 0;
		fl_num = 0;
		cnt = 0;

		ch = *fmt++;

		if( ch=='0' ) {
			fl_zero = 1;
			ch = *fmt++;
			cnt = ch - '0';
			ch = *fmt++;
		}
		else if( ch>='0' && ch<='9' ) {
			cnt = ch - '0';
			ch = *fmt++;
		}

		//----- Get char(B) / int / long(L) ----------------

		fl_len = 2;

		switch(ch) {
		case 'l':
		case 'L':	ch = *fmt++;	fl_len = 4;		break;
		case 'b':
		case 'B':	ch = *fmt++;	fl_len = 1;		break;
		}		

		//----- Get Type Discriptor -----
		
		switch( ch ) {

			case 'd':
			case 'u':

				switch (fl_len) {
				case 1:
					if( ch=='d' ) value = (char)va_arg( ap, char );
					else          value = (uint8_t)va_arg( ap, uint8_t );
					break;

				case 2:
					if( ch=='d' ) value = (int)va_arg( ap,  int );
					else          value = (uint16_t)va_arg( ap, uint16_t );
					break;

				case 4:
					if( ch=='d' ) value = (long)va_arg( ap, long );
					else          value = (uint32_t)va_arg( ap, uint32_t );
					break;
				}

				if( value<0 ) {
					_outchar('-');
					value = value*(-1);
				}

				if(cnt==0) {
					if( value==0 ) { _outchar('0'); continue; }

					for(cnt=0, mask=1; cnt<10; cnt++) {
						if( (value/mask)==0 ) break;
						mask = mask*10;
					}
				}

				for(i=0, mask=1; i<cnt-1; i++) mask = mask*10;

				while(1) {
					ch = (value / mask) + '0';
					if( ch=='0' && fl_zero==0 && mask!=1 ) ch=' ';
					else fl_zero = 1;
					_outchar(ch);

					value = value % (mask);
					mask = mask / 10;
					
					if( mask==0 )
						break;
				}
				continue;

			case 'x':
			case 'X':

				switch (fl_len) {
				case 1:	value = (uint8_t)va_arg( ap, uint8_t );		break;
				case 2:	value = (uint16_t)va_arg( ap, uint16_t );		break;
				case 4:	value = (uint32_t)va_arg( ap, uint32_t );		break;
				}

				if(cnt==0) 
					cnt = fl_len*2;

				global_cnt = cnt;
				global_value = value;
				for(i=0; i<cnt; i++) {
					_outchar( Hex[(value >> (cnt-i-1)*4) & 0x000f] );
				}
				continue;

			case 's':
				ptr = (char *)va_arg( ap, char* );
				while(*ptr!='\0')
					_outchar(*ptr++);
				continue;

			case 'c':
				value = va_arg( ap, int );
					_outchar((uint8_t)value);
				continue;

			default:
				value = (uint16_t)va_arg( ap, int );
				continue;
		}
	}
}
#endif


void Printf( const char CODE_P *fmt, ... )
{
#ifdef _DEBUG_MODE
	va_list ap;

	va_start(ap, fmt);
	DoPrint( fmt, ap );
	va_end( ap );
#endif
}

/*void Puts( char *ptr )
{
	while(*ptr!='\0')
		_outchar(*ptr++);
}*/
