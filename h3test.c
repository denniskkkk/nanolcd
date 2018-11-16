#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

int main () {
int gpioh;
int gpior;
#define BUFFER_MAX 3
#define DIRECTION_MAX 35
char buffer[BUFFER_MAX];
#define VALUE_MAX 30
char path[VALUE_MAX];
char path2[VALUE_MAX];
char tmpbuf[64];
int len;
long count;
	gpioh = open("/sys/class/gpio/export", O_WRONLY);
        if (gpioh < 0) {
                fprintf (stderr , "error open ");
                exit (-1);
        }
	len = snprintf(buffer, BUFFER_MAX, "%d", 6);
	write(gpioh, buffer, len);
	len = snprintf(buffer, BUFFER_MAX, "%d", 7);
	write(gpioh, buffer, len);
	close(gpioh);
	//  direction
	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio6/direction");
        gpioh = open(path, O_WRONLY);
        if (gpioh < 0) {
                fprintf (stderr , "error open ");
                exit (-1);
        }
        printf ("direction, %s\n", path);
        write(gpioh, "out", 3);
        close(gpioh);
	//
        gpioh = open(path, O_RDONLY);
        if (gpioh < 0) {
                fprintf (stderr , "error open ");
                exit (-1);
        }
        read(gpioh, tmpbuf, 32);
        printf ("read A6: %s\n", tmpbuf);
        close(gpioh);
	//
	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", 7);
	gpioh = open(path, O_WRONLY);
        if (gpioh < 0) {
                fprintf (stderr , "error open ");
                exit (-1);
        }
        printf ("direction, %s\n", path);
	write(gpioh, "in", 3);
	close(gpioh);
	//
        gpioh = open(path, O_RDONLY);
        if (gpioh < 0) {
                fprintf (stderr , "error open ");
                exit (-1);
        }
        read(gpioh, tmpbuf, 32);
        printf ("read A7: %s\n", tmpbuf);
        close(gpioh);
	// test output A6
	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", 6);
	snprintf(path2, VALUE_MAX, "/sys/class/gpio/gpio%d/value", 7);
	gpioh = open(path, O_WRONLY);
	//gpior = open(path2, O_RDONLY);
	printf ("value, %s\n", path);
	printf ("value, %s\n", path2);
	for (count = 0; count < 1000000000; count ++ )
	{
	write(gpioh, "0", 1 );
//	printf ("logic low\n");
//	usleep (1000000);
	write(gpioh, "1", 1 );
//	printf ("logic high\n");
//	usleep (1000000);

        gpior = open(path2, O_RDONLY);
	read (gpior, tmpbuf, 1);
 	close(gpior);
	printf ("key = %s", tmpbuf);

	}
	close(gpioh);
	//	close(gpior);

return 0;

}

