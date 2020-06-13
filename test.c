#include <stdio.h>

char           *
itof22(s, f)
    char           *s;
    double         f;
{
   register             shift = 0;

   if(f >= 100.){
      f /= 10.;
      shift = 1;
   }

   *s = '0' + ((int) f % 100) / 10;
   if (*s == '0' && (int) f < 100)
      *s = ' ';
   *++s = '0' + ((int) f % 10);
   if(!shift)
      *++s = '.';
   *++s = '0' + ((int) (f * 10)) % 10;
   if(shift)
      *++s = '.';
   *++s = '0' + ((int) (f * 100)) % 10;

   return ++s;
}

main()
{
   char	buf[80], *b;
   printf("\"%s\"\n", buf,b=itof22(buf, 0.2),*b=0);
   printf("\"%s\"\n", buf,b=itof22(buf, 4.2),*b=0);
   printf("\"%s\"\n", buf,b=itof22(buf, 40.2),*b=0);
   printf("\"%s\"\n", buf,b=itof22(buf, 400.25),*b=0);
   printf("\"%s\"\n", buf,b=itof22(buf, 4000.2),*b=0);
}
