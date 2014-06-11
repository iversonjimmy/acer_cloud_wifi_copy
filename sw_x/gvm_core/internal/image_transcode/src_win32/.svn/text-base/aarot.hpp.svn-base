#ifndef AAROT_HPP
#define AAROT_HPP

#include <windows.h>
#include <iostream>
#include <vector>
#include <math.h>

#define aar_abs(a) (((a) < 0)?(-(a)):(a))

typedef bool(* aar_callback) (double);

struct aar_pnt
{
	double x,y;
	inline aar_pnt(){}
    inline aar_pnt(double x,double y):x(x),y(y){}
};

struct aar_dblrgbquad
{
	double red, green, blue, alpha;
};

struct aar_indll
{
    aar_indll * next;
    int ind;
};

class aarot
{
public:
    HBITMAP rotate(HBITMAP, double, aar_callback, int, bool);
    HBITMAP rotate(HBITMAP, double, aar_callback);
    HBITMAP rotate(HBITMAP, double, aar_callback, int);
    HBITMAP rotate(HBITMAP, double, aar_callback, bool);

private:
    double coss;
    double sins;
    aar_pnt * polyoverlap;
    int polyoverlapsize;
    aar_pnt * polysorted;
    int polysortedsize;
    aar_pnt * corners;

    inline int roundup(double a) {if (aar_abs(a - (int)(a + 5e-10)) < 1e-9) return (int)(a + 5e-10); else return (int)(a + 1);}
    inline int round(double a) {return (int)(a + 0.5);}
    inline BYTE byterange(double a) {int b = round(a); if (b <= 0) return 0; else if (b >= 255) return 255; else return (BYTE)b;}
    inline double aar_min(double & a, double & b) {if (a < b) return a; else return b;}
    inline double aar_max(double & a, double & b) {if (a > b) return a; else return b;}

    inline double aar_cos(double);
    inline double aar_sin(double);

    inline double area();
    inline void sortpoints();
    inline bool isinsquare(aar_pnt, aar_pnt &);
    inline double pixoverlap(aar_pnt *, aar_pnt);

    HBITMAP dorotate(HBITMAP, double, aar_callback, int, bool);
};

//Prevent Float Errors with Cos and Sin
double aarot::aar_cos(double degrees)
{
    double ret;
    double off = (degrees / 30 - round(degrees / 30));
    if (off < .0000001 && off > -.0000001)
    {
        int idegrees = (int)round(degrees);
        idegrees = (idegrees < 0) ? (360 - (-idegrees % 360))  : (idegrees % 360);
        switch (idegrees)
        {
            case 0: ret=1.0; break;
            case 30: ret=0.866025403784439; break;
            case 60: ret=0.5; break;
            case 90: ret=0.0; break;
            case 120: ret=-0.5; break;
            case 150: ret=-0.866025403784439; break;
            case 180: ret=-1.0; break;
            case 210: ret=-0.866025403784439; break;
            case 240: ret=-0.5; break;
            case 270: ret=0.0; break;
            case 300: ret=0.5; break;
            case 330: ret=0.866025403784439; break;
            case 360: ret=1.0; break;
            default: ret=cos(degrees * 3.14159265358979 / 180);  // it shouldn't get here
        }
        return ret;
    }
    else
        return cos(degrees * 3.14159265358979 / 180);
}

double aarot::aar_sin(double degrees)
{
    return aar_cos(degrees + 90.0);
}

double aarot::area()
{
    double ret = 0.0;
    //Loop through each triangle with respect to (0, 0) and add the cross multiplication
    for (int i = 0; i + 1 < polysortedsize; i++)
        ret += polysorted[i].x * polysorted[i + 1].y - polysorted[i + 1].x * polysorted[i].y;
    //Take the absolute value over 2
    return aar_abs(ret) / 2.0;
}

void aarot::sortpoints()
{
    if (polyoverlapsize < 3)
        return;

    if (polyoverlapsize == 3)
    {
        polysortedsize = polyoverlapsize - 1;
        polysorted[0].x = polyoverlap[1].x - polyoverlap[0].x;
        polysorted[0].y = polyoverlap[1].y - polyoverlap[0].y;
        polysorted[1].x = polyoverlap[2].x - polyoverlap[0].x;
        polysorted[1].y = polyoverlap[2].y - polyoverlap[0].y;
        return;
    }

    aar_indll * root = new aar_indll;
    root->next = NULL;

    for (int i = 1; i < polyoverlapsize; i++)
    {
        polyoverlap[i].x = polyoverlap[i].x - polyoverlap[0].x;
        polyoverlap[i].y = polyoverlap[i].y - polyoverlap[0].y;

        aar_indll * node = root;
        while (true)
        {
            if (node->next)
            {
                if (polyoverlap[i].x * polyoverlap[node->next->ind].y - polyoverlap[node->next->ind].x * polyoverlap[i].y < 0)
                {
                    aar_indll * temp = node->next;
                    node->next = new aar_indll;
                    node->next->ind = i;
                    node->next->next = temp;
                    break;
                }
            }
            else
            {
                node->next = new aar_indll;
                node->next->ind = i;
                node->next->next = NULL;
                break;
            }
            node = node->next;
        }
    }

    //We can leave out the first point because it's offset position is going to be (0, 0)
    polysortedsize = 0;

    aar_indll * node = root;
    aar_indll * temp;
    while (node)
    {
        temp = node;
        node = node->next;
        if (node)
            polysorted[polysortedsize++] = polyoverlap[node->ind];
        delete temp;
    }
}

bool aarot::isinsquare(aar_pnt r, aar_pnt & c)
{
    //Offset r
    r.x -= c.x;
    r.y -= c.y;

    //rotate r
    aar_pnt nr;
    nr.x = r.x * coss + r.y * sins;
    nr.y = r.y * coss - r.x * sins;

    //Find if the rotated polygon is within the square of size 1 centerd on the origin
    nr.x = aar_abs(nr.x);
    nr.y = aar_abs(nr.y);
    return (nr.x < 0.5 && nr.y < 0.5);
}

double aarot::pixoverlap(aar_pnt * p, aar_pnt c)
{
    polyoverlapsize = 0;
    polysortedsize = 0;

    int ja [] = {1, 2, 3, 0};
    double minx, maxx, miny, maxy;
    int j;

    double z;

    for (int i = 0; i < 4; i++)
    {        
        //Search for source points within the destination square
        if (p[i].x >= 0 && p[i].x <= 1 && p[i].y >= 0 && p[i].y <= 1)
            polyoverlap[polyoverlapsize++] = p[i];

        //Search for destination points within the source square
        if (isinsquare(corners[i], c))
            polyoverlap[polyoverlapsize++] = corners[i];

        //Search for line intersections
        j = ja[i];
        minx = aar_min(p[i].x, p[j].x);
        miny = aar_min(p[i].y, p[j].y);
        maxx = aar_max(p[i].x, p[j].x);
        maxy = aar_max(p[i].y, p[j].y);

        if (minx < 0.0 && 0.0 < maxx)
        {//Cross left
            z = p[i].y - p[i].x * (p[i].y - p[j].y) / (p[i].x - p[j].x);
            if (z >= 0.0 && z <= 1.0)
            {
                polyoverlap[polyoverlapsize].x = 0.0;
                polyoverlap[polyoverlapsize++].y = z;
            }
        }
        else if (minx < 1.0 && 1.0 < maxx)
        {//Cross right
            z = p[i].y + (1 - p[i].x) * (p[i].y - p[j].y) / (p[i].x - p[j].x);
            if (z >= 0.0 && z <= 1.0)
            {
                polyoverlap[polyoverlapsize].x = 1.0;
                polyoverlap[polyoverlapsize++].y = z;
            }
        }
        if (miny < 0.0 && 0.0 < maxy)
        {//Cross bottom
            z = p[i].x - p[i].y * (p[i].x - p[j].x) / (p[i].y - p[j].y);
            if (z >= 0.0 && z <= 1.0)
            {
                polyoverlap[polyoverlapsize].x = z;
                polyoverlap[polyoverlapsize++].y = 0.0;
            }
        }
        else if (miny < 1.0 && 1.0 < maxy)
        {//Cross top
            z = p[i].x + (1 - p[i].y) * (p[i].x - p[j].x) / (p[i].y - p[j].y);
            if (z >= 0.0 && z <= 1.0)
            {
                polyoverlap[polyoverlapsize].x = z;
                polyoverlap[polyoverlapsize++].y = 1.0;
            }
        }
    }        

    //Sort the points and return the area
    sortpoints();
    return area();
}

HBITMAP aarot::dorotate(HBITMAP src, double rotation, aar_callback callbackfunc, int bgcolor, bool autoblend)
{
    //Calculate some index values so that values can easily be looked up
    int indminx = ((int)rotation / 90 + 0) % 4;
    int indminy = (indminx + 1) % 4;
    int indmaxx = (indminx + 2) % 4;
    int indmaxy = (indminx + 3) % 4;

    //Load the source bitmaps information
    BITMAP srcbmp;
    if (GetObject(src, sizeof(srcbmp), &srcbmp) == 0)
		return NULL;

    //Calculate the sources x and y offset
    double srcxres = (double)srcbmp.bmWidth / 2.0;
    double srcyres = (double)srcbmp.bmHeight / 2.0;

    //Calculate the x and y offset of the rotated image (half the width and height of the rotated image)
    int mx[] = {-1, 1, 1, -1};
    int my[] = {-1, -1, 1, 1};
    double xres = mx[indmaxx] * srcxres * coss - my[indmaxx] * srcyres * sins;
    double yres = mx[indmaxy] * srcxres * sins + my[indmaxy] * srcyres * coss;

    //Get the width and height of the image
    int width = roundup(xres * 2);
    int height = roundup(yres * 2);

    //Create the source dib array and the destdib array
    RGBQUAD * srcdib = new RGBQUAD[srcbmp.bmWidth * srcbmp.bmHeight];
    aar_dblrgbquad * dbldstdib = new aar_dblrgbquad[width * height];
	memset(dbldstdib, 0, width * height * sizeof(aar_dblrgbquad));

    //Load source bits into srcdib
    BITMAPINFO srcdibbmap;
    srcdibbmap.bmiHeader.biSize = sizeof(srcdibbmap.bmiHeader);
    srcdibbmap.bmiHeader.biWidth = srcbmp.bmWidth;
    srcdibbmap.bmiHeader.biHeight = -srcbmp.bmHeight;
    srcdibbmap.bmiHeader.biPlanes = 1;
    srcdibbmap.bmiHeader.biBitCount = 32;
    srcdibbmap.bmiHeader.biCompression = BI_RGB;

    HDC ldc = CreateCompatibleDC(0);
    GetDIBits(ldc, src, 0, srcbmp.bmHeight, srcdib, &srcdibbmap, DIB_RGB_COLORS);
    DeleteDC(ldc);

    
    aar_pnt * p = new aar_pnt[4];
    aar_pnt * poffset = new aar_pnt[4];
    aar_pnt c(0, 0);

    //Loop through the source's pixels
    double xtrans;
    double ytrans;
    for (int x = 0; x < srcbmp.bmWidth; x++)
    {
        for (int y = 0; y < srcbmp.bmHeight; y++)
        {
            //Construct the source pixel's rotated polygon
            
            xtrans = (double)x - srcxres;
            ytrans = (double)y - srcyres;

            p[0].x = xtrans * coss - ytrans * sins + xres;
            p[0].y = xtrans * sins + ytrans * coss + yres;
            p[1].x = (xtrans + 1.0) * coss - ytrans * sins + xres;
            p[1].y = (xtrans + 1.0) * sins + ytrans * coss + yres;
            p[2].x = (xtrans + 1.0) * coss - (ytrans + 1.0) * sins + xres;
            p[2].y = (xtrans + 1.0) * sins + (ytrans + 1.0) * coss + yres;
            p[3].x = xtrans * coss - (ytrans + 1.0) * sins + xres;
            p[3].y = xtrans * sins + (ytrans + 1.0) * coss + yres;

            //Caculate center of the polygon
            c.x = 0;
            c.y = 0;
            for (int i = 0; i < 4; i++)
            {
                c.x += p[i].x / 4.0;
                c.y += p[i].y / 4.0;
            }

            //Find the scan area on the destination's pixels
            int mindx = (int)p[indminx].x;
            int mindy = (int)p[indminy].y;
            int maxdx = roundup(p[indmaxx].x);
            int maxdy = roundup(p[indmaxy].y);
            
            int SrcIndex = x + y * srcbmp.bmWidth;
            //loop through the scan area to find where source(x, y) overlaps with the destination pixels

            for (int xx = mindx; xx < maxdx; xx++)
            {
                for (int yy = mindy; yy < maxdy; yy++)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        poffset[i].x = p[i].x - xx;
                        poffset[i].y = p[i].y - yy;
                    }

                    //Calculate the area of the source's rotated pixel (polygon p) over the destinations pixel (xx, yy)
                    //The function actually calculates the area of poffset over the square (0,0)-(1,1)
                    double dbloverlap = pixoverlap(poffset, aar_pnt(c.x - xx, c.y - yy));
                    if (dbloverlap)
                    {
                        int DstIndex = xx + yy * width;

                        //Add the rgb and alpha values in proportion to the overlap area
                        dbldstdib[DstIndex].red += (double)(srcdib[SrcIndex].rgbRed) * dbloverlap;
                        dbldstdib[DstIndex].blue += (double)(srcdib[SrcIndex].rgbBlue) * dbloverlap;
                        dbldstdib[DstIndex].green += (double)(srcdib[SrcIndex].rgbGreen) * dbloverlap;
                        dbldstdib[DstIndex].alpha += dbloverlap;
                    }
                }
            }
            
        }
        if (callbackfunc != NULL)
        {
            double percentdone = (double)(x + 1) / (double)(srcbmp.bmWidth);
            if (callbackfunc(percentdone))
            {
                delete [] srcdib;
                delete [] dbldstdib;
                return NULL;
            }
        }
    }
    delete [] p;
    delete [] poffset;
    delete [] srcdib;
    srcdib = NULL;

    //Create final destination bits
    RGBQUAD * dstdib = new RGBQUAD[width * height];

    //load dstdib with information from dbldstdib
    RGBQUAD backcolor;
    backcolor.rgbRed = bgcolor & 0x000000FF;
    backcolor.rgbGreen = (bgcolor & 0x0000FF00) / 0x00000100;
    backcolor.rgbBlue = (bgcolor & 0x00FF0000) / 0x00010000;
    for (int i = 0; i < width * height; i++)
    {
        if (dbldstdib[i].alpha)
        {
            if (autoblend)
            {
                dstdib[i].rgbReserved = 0;
                dstdib[i].rgbRed = byterange(dbldstdib[i].red + (1 - dbldstdib[i].alpha) * (double)backcolor.rgbRed);
                dstdib[i].rgbGreen = byterange(dbldstdib[i].green + (1 - dbldstdib[i].alpha) * (double)backcolor.rgbGreen);
                dstdib[i].rgbBlue = byterange(dbldstdib[i].blue + (1 - dbldstdib[i].alpha) * (double)backcolor.rgbBlue);
            }
            else
            {
                dstdib[i].rgbRed = byterange(dbldstdib[i].red / dbldstdib[i].alpha);
                dstdib[i].rgbGreen = byterange(dbldstdib[i].green / dbldstdib[i].alpha);
                dstdib[i].rgbBlue = byterange(dbldstdib[i].blue / dbldstdib[i].alpha);
                dstdib[i].rgbReserved = byterange(255.0 * dbldstdib[i].alpha);
            }
        }
        else
        {
            //No color information
            dstdib[i].rgbRed = backcolor.rgbRed;
            dstdib[i].rgbGreen = backcolor.rgbGreen;
            dstdib[i].rgbBlue = backcolor.rgbBlue;
            dstdib[i].rgbReserved = 0;
        }
    }

    delete [] dbldstdib;
    dbldstdib = NULL;

    //Get Current Display Settings
    DEVMODE screenmode;
    screenmode.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &screenmode);

    //Create the final bitmap object
    HBITMAP dstbmp = CreateBitmap(width, height, 1, screenmode.dmBitsPerPel, NULL);

    //Write the bits into the bitmap and return it
    BITMAPINFO dstdibmap;
    dstdibmap.bmiHeader.biSize = sizeof(dstdibmap.bmiHeader);
    dstdibmap.bmiHeader.biWidth = width;
    dstdibmap.bmiHeader.biHeight = -height;
    dstdibmap.bmiHeader.biPlanes = 1;
    dstdibmap.bmiHeader.biBitCount = 32;
    dstdibmap.bmiHeader.biCompression = BI_RGB;
    SetDIBits(0, dstbmp, 0, height, dstdib, &dstdibmap, DIB_RGB_COLORS);
    
    delete [] dstdib;
    dstdib = NULL;

    return dstbmp;
}

HBITMAP aarot::rotate(HBITMAP src, double rotation, aar_callback callbackfunc, int bgcolor, bool autoblend)
{
    polyoverlap = new aar_pnt[16];
    polysorted = new aar_pnt[16];
    corners = new aar_pnt[4];
    
    double dx[] = {0.0, 1.0, 1.0, 0.0};
    double dy[] = {0.0, 0.0, 1.0, 1.0};
    for (int i = 0; i < 4; i++)
    {
        corners[i].x = dx[i];
        corners[i].y = dy[i];
    }

    //Get rotation between [0, 360)
    int mult = (int)rotation / 360;
    if (rotation >= 0)
        rotation = rotation - 360.0 * mult;
    else
        rotation = rotation - 360.0 * (mult - 1);

    //Calculate the cos and sin values that will be used throughout the program
    coss = aar_cos(rotation);
    sins = aar_sin(rotation);    

    HBITMAP res = dorotate(src, rotation, callbackfunc, bgcolor, autoblend);

    delete [] polyoverlap;
    delete [] polysorted;
    delete [] corners;

    return res;
}

HBITMAP aarot::rotate(HBITMAP src, double rotation, aar_callback callbackfunc)
{
    return rotate(src, rotation, callbackfunc, 0x00FFFFFF, true);
}

HBITMAP aarot::rotate(HBITMAP src, double rotation, aar_callback callbackfunc, int bgcolor)
{
    return rotate(src, rotation, callbackfunc, bgcolor, true);
}

HBITMAP aarot::rotate(HBITMAP src, double rotation, aar_callback callbackfunc, bool autoblend)
{
    return rotate(src, rotation, callbackfunc, 0x00FFFFFF, autoblend);
}
#endif
