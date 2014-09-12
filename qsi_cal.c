#include <stdio.h>
#include <stdlib.h>

typedef struct {
	int x[5], xfb[5];
	int y[5], yfb[5];
	int a[7];
} calibration;

int perform_calibration(calibration *cal) {
	int j;
	float n, x, y, x2, y2, xy, z, zx, zy;
	float det, a, b, c, e, f, i;
	float scaling = 65536.0;

// Get sums for matrix
	n = x = y = x2 = y2 = xy = 0;
	for(j=0;j<5;j++) {
		n += 1.0;
		x += (float)cal->x[j];
		y += (float)cal->y[j];
		x2 += (float)(cal->x[j]*cal->x[j]);
		y2 += (float)(cal->y[j]*cal->y[j]);
		xy += (float)(cal->x[j]*cal->y[j]);
	}

// Get determinant of matrix -- check if determinant is too small
	det = n*(x2*y2 - xy*xy) + x*(xy*y - x*y2) + y*(x*xy - y*x2);
	if(det < 0.1 && det > -0.1) {
		printf("ts_calibrate: determinant is too small -- %f\n",det);
		return 0;
	}

// Get elements of inverse matrix
	a = (x2*y2 - xy*xy)/det;
	b = (xy*y - x*y2)/det;
	c = (x*xy - y*x2)/det;
	e = (n*y2 - y*y)/det;
	f = (x*y - n*xy)/det;
	i = (n*x2 - x*x)/det;

// Get sums for x calibration
	z = zx = zy = 0;
	for(j=0;j<5;j++) {
		z += (float)cal->xfb[j];
		zx += (float)(cal->xfb[j]*cal->x[j]);
		zy += (float)(cal->xfb[j]*cal->y[j]);
	}

// Now multiply out to get the calibration for framebuffer x coord
	cal->a[0] = (int)((a*z + b*zx + c*zy)*(scaling));
	cal->a[1] = (int)((b*z + e*zx + f*zy)*(scaling));
	cal->a[2] = (int)((c*z + f*zx + i*zy)*(scaling));

	printf("%f %f %f\n",(a*z + b*zx + c*zy),
				(b*z + e*zx + f*zy),
				(c*z + f*zx + i*zy));

// Get sums for y calibration
	z = zx = zy = 0;
	for(j=0;j<5;j++) {
		z += (float)cal->yfb[j];
		zx += (float)(cal->yfb[j]*cal->x[j]);
		zy += (float)(cal->yfb[j]*cal->y[j]);
	}

// Now multiply out to get the calibration for framebuffer y coord
	cal->a[3] = (int)((a*z + b*zx + c*zy)*(scaling));
	cal->a[4] = (int)((b*z + e*zx + f*zy)*(scaling));
	cal->a[5] = (int)((c*z + f*zx + i*zy)*(scaling));

	printf("%f %f %f\n",(a*z + b*zx + c*zy),
				(b*z + e*zx + f*zy),
				(c*z + f*zx + i*zy));

// If we got here, we're OK, so assign scaling to a[6] and return
	cal->a[6] = (int)scaling;
	return 1;
}

void set_calibration(calibration* cal, int i, int xfb, int yfb, int x, int y)
{
    cal->x[i]=x;
    cal->y[i]=y;
    cal->xfb[i]=xfb;
    cal->yfb[i]=yfb;
}

int main()
{
    calibration cal;

    int fb_x, fb_y;

    fb_x=800;
    fb_y=480;

    set_calibration(&cal, 0, 50,      50,      70,  58);    // Top left
    set_calibration(&cal, 1, fb_x-50, 50,      75,  909);   // Top right
    set_calibration(&cal, 2, fb_x-50, fb_y-50, 545, 915);   // Bot right
    set_calibration(&cal, 3, 50,      fb_y-50, 545, 50);    // Bot left
    set_calibration(&cal, 4, fb_x/2,  fb_y/2,  310, 480);   // Center

	if (perform_calibration (&cal)) {
		printf ("Calibration constants: ");
		printf ("%d %d %d %d %d %d %d\n",
			 cal.a[1], cal.a[2], cal.a[0],
			 cal.a[4], cal.a[5], cal.a[3], cal.a[6]);
	} else {
		printf("Calibration failed.\n");
	}
    return 0;
}

