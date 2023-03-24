#include "utility.h"

void RGB2HSV(RGB rgb, int &_H, int &_S, int &_V)
{// in our case R <->B

    unsigned char _R = rgb.red, _G = rgb.green, _B = rgb.blue;

    double h, rc, gc, bc, dmax;

    unsigned char s, v;

    unsigned char minc, maxc;

    maxc = (_R > _G) ? ((_R > _B) ? _R : _B) : ((_G > _B) ? _G : _B);

    minc = (_R < _G) ? ((_R < _B) ? _R : _B) : ((_G < _B) ? _G : _B);

    s = 0; // Saturation



    if (maxc)

    {

        s = (maxc - minc) * 255 / maxc;

    }

    int sat = s;

    int val = maxc;

    int hue = 0;

    if (!s)

    { // Achromatic color

        hue = 0;

    }

    else

    { // Chromatic color

        dmax = maxc - minc;

        rc = (maxc - _R) / dmax; /* rc - distance */

        gc = (maxc - _G) / dmax; /* from red */

        bc = (maxc - _B) / dmax;

        if (_R == maxc)

        { // Color between yellow and purple

            h = bc - gc;

        }

        else if (_G == maxc)

        { // Color between blue and yellow

            h = 2 + rc - bc;

        }

        else

        { // Color between purple and blue

            h = 4 + gc - rc;

        }

        h *= 60.0;

        if (h < 0.0)

        {

            h += 360.0;

        }

        hue = h;

        if (hue == 360)

        {

            hue = 0;

        }

    } // if (!s)
    _H = hue;
    _S = sat;
    _V = val;

}

// source: https://www.compuphase.com/cmetric.htm
double calcColorDistance(const RGB &rgb1, const RGB &rgb2)
{
    long rmean = ( (long)rgb1.red + (long)rgb2.red ) / 2;
    long r = (long)rgb1.red - (long)rgb2.red;
    long g = (long)rgb1.green - (long)rgb2.green;
    long b = (long)rgb1.blue - (long)rgb2.blue;
    return sqrt((((512+rmean)*r*r)>>8) + 4*g*g + (((767-rmean)*b*b)>>8));
}

double calcVariance(std::vector<double> &v)
{
    double mean{0}, variance{0};
    if (v.size() == 0) return 0;
    for (auto k : v) {
        mean+= k;
    }
    mean /= v.size();
    for (auto k : v) {
        variance += pow(mean - k, 2);
    }
    variance = sqrt(variance / (v.size()-1));
    return variance;
}
