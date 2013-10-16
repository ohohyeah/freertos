#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <stdarg.h>

#define ALIGN (sizeof(size_t))
#define ONES ((size_t)-1/UCHAR_MAX)                                                                      
#define HIGHS (ONES * (UCHAR_MAX/2+1))
#define HASZERO(x) ((x)-ONES & ~(x) & HIGHS)

#define SS (sizeof(size_t))
void *memset(void *dest, int c, size_t n)
{
	unsigned char *s = dest;
	c = (unsigned char)c;
	for (; ((uintptr_t)s & ALIGN) && n; n--) *s++ = c;
	if (n) {
		size_t *w, k = ONES * c;
		for (w = (void *)s; n>=SS; n-=SS, w++) *w = k;
		for (s = (void *)w; n; n--, s++) *s = c;
	}
	return dest;
}

void *memcpy(void *dest, const void *src, size_t n)
{
	void *ret = dest;
	
	//Cut rear
	uint8_t *dst8 = dest;
	const uint8_t *src8 = src;
	switch (n % 4) {
		case 3 : *dst8++ = *src8++;
		case 2 : *dst8++ = *src8++;
		case 1 : *dst8++ = *src8++;
		case 0 : ;
	}
	
	//stm32 data bus width
	uint32_t *dst32 = (void *)dst8;
	const uint32_t *src32 = (void *)src8;
	n = n / 4;
	while (n--) {
		*dst32++ = *src32++;
	}
	
	return ret;
}

char *strchr(const char *s, int c)
{
	for (; *s && *s != c; s++);
	return (*s == c) ? (char *)s : NULL;
}

char *strcpy(char *dest, const char *src)
{
	const unsigned char *s = src;
	unsigned char *d = dest;
	while ((*d++ = *s++));
	return dest;
}

char *strncpy(char *dest, const char *src, size_t n)
{
	const unsigned char *s = src;
	unsigned char *d = dest;
	while (n-- && (*d++ = *s++));
	return dest;
}


void itoa(int n, char *out)
{
	int sign =0;
	if(n< 0) sign =1;
	int i = 0;
	if( n == 0)
		out[i++] =0;
	else if (n <0 )
	{
		out[i++] = '-';
		n = -n;
	}		
	while (n >0 )
	{
		out[i++] = '0' + (n %10);
		n /=10;
	}
	out[i] ='\0';

	/* reverse the array*/
	int j = 0 + sign, k = i- 1 - sign; 
	char temp;
	while(j < k)
	{
		temp = out[j];
		out[j] = out[k];
		out[k] = temp;
		j++;k--;
	}
}

size_t strlen(const char *string)
{
    int chars = 0;

    while(*string++) {
        chars++;
    }
    return chars;
}



int strcmp(const char *a, const char *b)
{
    int i = 0;

    while(a[i]) 	
    {
        if (a[i] != b[i]) 
	{
            return a[i] - b[i];
        }
        i++;
    }
    return 0;
}

void myprintf(const char *fmt, ...)
{
		
	va_list args;
 	va_start(args, fmt);
	char out[100];
	int count =0;
	for(count =0;fmt[count] != '\0'; count++)
	{	
		if(fmt[count] == '%')
		{
			switch(fmt[++count])
			{
				case 'NULL':
				continue;
				case 's':
				send_msg(va_arg(args, char *));	
				break;
				case 'd':
				itoa(va_arg(args, int), out);
				send_msg(out);
				break;
				default:
				send_msg(fmt[count]);
			}
					
		}
	
		else
		{
						
			send_byte(fmt[count]);
		
		}
		
	}
	 va_end(args);
}

