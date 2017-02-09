#include "io.h"

#include <string>
using std::string;

using std::tuple;
using std::make_tuple;
using std::tie;
/*
Image load_image(const char *path)
{
    BMP in;

    if (!in.ReadFromFile(path))
        throw string("Error reading file ") + string(path);

    Image res(in.TellHeight(), in.TellWidth());

    for (uint i = 0; i < res.n_rows; ++i) {
        for (uint j = 0; j < res.n_cols; ++j) {
            RGBApixel *p = in(j, i);
            res(i, j) = make_tuple(p->Red, p->Green, p->Blue);
        }
    }

    return res;
}
 */

void save_image(const Image &im, const char *path)
{
    BMP out;
    out.SetSize(im.n_cols, im.n_rows);

    uint c;
    RGBApixel p;
    p.Alpha = 255;
    for (uint i = 0; i < im.n_rows; ++i) {
        for (uint j = 0; j < im.n_cols; ++j) {
            c = im(i, j);
            p.Red = c; p.Green = c; p.Blue = c;
            out.SetPixel(j, i, p);
        }
    }

    std::cout << "sfsf"<< std::endl;
    /*
    if (!out.WriteToFile(path))
        std::cout << "sfsfedsgsdg"<< std::endl;
        throw string("Error writing file ") + string(path);
        */
    out.WriteToFile(path);
}