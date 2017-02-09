#include "align.h"
#include <string>

using std::string;
using std::cout;
using std::endl;


long double MSE (Image img1, Image img2,
                 std::tuple <uint, uint> left_up_img1,
                 std::tuple <uint, uint> right_down_img1,
                 std::tuple <uint, uint> left_up_img2, 
                 uint stage)
{

    uint border = uint(0.03 * (img1.n_rows));
    long double error = 0;
    uint i2, j2;
    i2 = std::get<0>(left_up_img2) + border;
    j2 = std::get<1>(left_up_img2) + border;   

    for (uint i1 = std::get<0>(left_up_img1) + border; i1 <= std::get<0>(right_down_img1) - border; ++i1) {
        for (uint j1 = std::get<1>(left_up_img1) + border; j1 <= std::get<1>(right_down_img1) - border; ++j1) {

            if (stage == 0) {
                error += (std::get<0>(img1(i1, j1)) - std::get<0>(img2(i2, j2)))
                * (std::get<0>(img1(i1, j1)) - std::get<0>(img2(i2, j2)));

            } else {
                error += (std::get<1>(img1(i1, j1)) - std::get<1>(img2(i2, j2)))
                * (std::get<1>(img1(i1, j1)) - std::get<1>(img2(i2, j2))) + 
                (std::get<2>(img1(i1, j1)) - std::get<2>(img2(i2, j2)))
                * (std::get<2>(img1(i1, j1)) - std::get<2>(img2(i2, j2)));
            }
             ++j2; 
        }
        ++i2;
        j2 = std::get<1>(left_up_img2) + border;
    }

    uint height = std::get<0>(right_down_img1) - std::get<0>(left_up_img1) + 1;
    uint width = std::get<1>(right_down_img1) - std::get<1>(left_up_img1) + 1;

    return error / (height * width);
}


Image Union (Image img1, Image img2, 
                std::tuple <std::tuple<uint, uint, uint, uint>,std::tuple<uint, uint>>  best_shift_1, 
                uint stage)
{
    uint left_str1, left_col1, i2, j2, right_str1, right_col1;
    std::tie(left_str1, left_col1, i2, j2) = std::get<0>(best_shift_1);
    std::tie(right_str1, right_col1) = std::get<1>(best_shift_1);

    for (uint i1 = left_str1; i1 <= right_str1; ++i1) {
        for (uint j1 = left_col1; j1 <= right_col1; ++j1) {

            if (stage == 0) {
                std::get<1>(img1(i1, j1)) = std::get<1>(img2(i2, j2));
            } else {
                std::get<0>(img1(i1, j1)) = std::get<0>(img2(i2, j2));
            }
            
            ++j2;
        }
        ++i2;
        j2 = std::get<3>(std::get<0>(best_shift_1));
    }

    return img1;
 } 


std::tuple <std::tuple<uint, uint, uint, uint>,std::tuple<uint, uint>> 
Search_Shift (Image img1, Image img2, uint stage)
{
    long double min_error;
    uint flag = 0;
    long double tmp;
    std::tuple<uint, uint, uint, uint> best_left;
    std::tuple<uint, uint> best_right;
    uint left_str1, left_str2, left_col1, left_col2; 
    uint right_str1, right_col1; 
    int shift = 0.04 * std::max(img1.n_rows, img1.n_cols);
    for (int i = (-1) * shift; i < shift; ++i) { 
        for (int j = (-1) * shift; j < shift; ++j) { 
            if (i >= 0) {
                left_str1 = i;
                left_str2 = 0;
                right_str1 = img1.n_rows - 1; 
            } else {
                left_str1 = 0;
                left_str2 = (-1) * i;
                right_str1 = img2.n_rows - (-1) * i - 1; 
            }

             if (j >= 0) {
                left_col1 = j;
                left_col2 = 0;
                right_col1 = img1.n_cols - 1; 
            } else {
                left_col1 = 0;
                left_col2 = (-1) * j;
                right_col1 = img2.n_cols - (-1) * j - 1; 
            }

            if (stage == 0) {
                tmp = MSE(img1, img2, std::make_tuple(left_str1, left_col1), 
                    std::make_tuple(right_str1, right_col1), 
                    std::make_tuple(left_str2, left_col2), 
                    0);
            } else {
                tmp = MSE(img1, img2, std::make_tuple(left_str1, left_col1), 
                    std::make_tuple(right_str1, right_col1), 
                    std::make_tuple(left_str2, left_col2), 
                    1);
            }
            

            if ((flag == 0) || (min_error > tmp) ){
                ++flag;
                min_error = tmp;
                best_left = std::make_tuple(left_str1, left_col1, left_str2, left_col2);
                best_right = std::make_tuple(right_str1, right_col1);

            }

        }
    }

    return std::make_tuple(best_left, best_right);
}

Image Mirror_Add_Frame (Image img, uint radius) 
{
    uint i, j;
    Image res(img.n_rows + 2*radius, img.n_cols + 2*radius);
    uint frame_coord; 

    for (i = radius; i < img.n_rows + radius; ++i) {
        for (j = radius; j < img.n_cols + radius; ++j){
            res(i, j) = img(i - radius, j  - radius);
        }
    }

    for (i = radius; i < img.n_rows + radius; ++i) {
        for (j = 0; j < radius; ++j) {
            res(i, j) = res(i, 2 * radius - j);
        }
        for (j = res.n_cols - radius; j < res.n_cols; ++j) {
            frame_coord = j - (radius + img.n_cols);
            res(i, j) = res(i, img.n_cols + radius - frame_coord - 2);
        }
    }

    for (j = radius; j < img.n_cols + radius; ++j) {
        for (i = 0; i < radius; ++i) {
            res(i, j) = res(2 * radius - i, j);
        }
        for (i = res.n_rows - radius; i < res.n_rows; ++i) {
            frame_coord = i - (radius + img.n_rows);
            res(i, j) = res(img.n_rows + radius - frame_coord - 2, j);
        }
    }

    uint centre_str, centre_col;
    uint shift_str, shift_col;

    for (i = 0; i < radius; ++i) {
        for (j = 0; j < radius; ++j) {
            centre_str = radius;
            centre_col = radius;
            shift_str = i - centre_str;
            shift_col = j - centre_col;

            res(i, j) = res(centre_str - shift_str, centre_col - shift_col);

            centre_str = radius;
            centre_col = res.n_cols - radius - 1;
            shift_str = i - centre_str;
            shift_str = img.n_cols + radius + j - centre_col;

            res(i, img.n_cols + radius + j) = res(centre_str - shift_str, centre_col - shift_col);

            centre_str = res.n_rows - radius - 1;
            centre_col = res.n_cols - radius - 1;
            shift_str = img.n_rows + radius + i - centre_str;
            shift_str = img.n_cols + radius + j - centre_col;

            res(img.n_rows + radius + i, img.n_cols + radius + j) = res(centre_str - shift_str, centre_col - shift_col);


            centre_str = res.n_rows - radius - 1;
            centre_col = radius;
            shift_str = img.n_rows + radius + i - centre_str;
            shift_str = j- centre_col;

            res(img.n_rows + radius + i, j) = res(centre_str - shift_str, centre_col - shift_col);

        }
    }
    return res;
}

Image Mirror_Drop_Frame(Image img, uint radius)
{
    return img.submatrix(radius, radius, img.n_rows - 2 * radius,  img.n_cols - 2 * radius);
}

Image align(Image srcImage, bool isPostprocessing, std::string postprocessingType, double fraction, bool isMirror, 
            bool isInterp, bool isSubpixel, double subScale)
{

    Image img1 = srcImage.submatrix(0, 0, srcImage.n_rows / 3.0, srcImage.n_cols);
    Image img2 = srcImage.submatrix(srcImage.n_rows / 3.0 - 1, 0, srcImage.n_rows / 3.0, srcImage.n_cols);
    Image img3 = srcImage.submatrix(2 * srcImage.n_rows / 3.0 - 1, 0, srcImage.n_rows / 3.0, srcImage.n_cols);


    std::tuple <std::tuple<uint, uint, uint, uint>,std::tuple<uint, uint>>  best_shift_1, best_shift_2;
    best_shift_1 = Search_Shift(img1, img2, 0);
    img1 = Union(img1, img2, best_shift_1, 0);
    best_shift_2 = Search_Shift(img1, img3, 1);
    img1 =  Union(img1, img3, best_shift_2, 1);

    if ((isPostprocessing) && (postprocessingType == "--gray-world")) {
        img1 = gray_world(img1); 
    }

    if ((isPostprocessing) && (postprocessingType == "--autocontrast")) {
        img1 = autocontrast(img1, 0.4); 
    }

    if ((isPostprocessing) && (postprocessingType == "--unsharp")) {
        img2 = unsharp(img2); 
    }
    
    return img1;

}

std::tuple <uint, uint, uint> check (long double r, long double g, long double b)
{
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (r > 256) r = 255;
    if (g > 256) g = 255;
    if (b > 256) b = 255;

    return std::make_tuple(r, g, b);
}



Image sobel_x(Image src_image) {
    Matrix<double> kernel = {{-1, 0, 1},
                             {-2, 0, 2},
                             {-1, 0, 1}};
    return custom(src_image, kernel);
}

Image sobel_y(Image src_image) {
    Matrix<double> kernel = {{ 1,  2,  1},
                             { 0,  0,  0},
                             {-1, -2, -1}};
    return custom(src_image, kernel);
}

class UnsharpFilterOp
{
public:
    std::tuple<uint, uint, uint> operator () (const Image &m) const
    {
        long double r_res, g_res, b_res;
        uint r1, r2, r3, r4, r5, r6, r7, r8, r9;
        uint g1, g2, g3, g4, g5, g6, g7, g8, g9;
        uint b1, b2, b3, b4, b5, b6, b7, b8, b9;
        std::tie(r1, g1, b1) = m(0, 0);
        std::tie(r2, g2, b2) = m(0, 1);
        std::tie(r3, g3, b3) = m(0, 2);
        std::tie(r4, g4, b4) = m(1, 0);
        std::tie(r5, g5, b5) = m(1, 1);
        std::tie(r6, g6, b6) = m(1, 2);
        std::tie(r7, g7, b7) = m(2, 0);
        std::tie(r8, g8, b8) = m(2, 1);
        std::tie(r9, g9, b9) = m(2, 2);

        r_res = ((-1.0 / 6) * (r1 + r3 + r7 + r9) + (-2.0 / 3) * (r2 + r4 + r6 + r8) + (13.0 / 3) * r5);
        g_res = ((-1.0 / 6) * (g1 + g3 + g7 + g9) + (-2.0 / 3) * (g2 + g4 + g6 + g8) + (13.0 / 3) * g5);
        b_res = ((-1.0 / 6) * (b1 + b3 + b7 + b9) + (-2.0 / 3) * (b2 + b4 + b6 + b8) + (13.0 / 3) * b5);
        return check(r_res, g_res, b_res);
    }
    static const int radius = 1;
};

Image unsharp(Image src_image) {
    Image img = Mirror_Add_Frame(src_image, 15);
    Image res = img.unary_map(UnsharpFilterOp());
    src_image = Mirror_Drop_Frame(res, 15);
    return src_image;
}


Image gray_world(Image src_image) {
    long double R_average  = 0, G_average = 0, B_average  = 0;
    for (uint str  = 0; str < src_image.n_rows; ++str){
        for (uint col = 0; col < src_image.n_cols; ++col) {
            R_average += std::get<0>(src_image(str, col));
            G_average += std::get<1>(src_image(str, col));
            B_average += std::get<2>(src_image(str, col));
        }
    }

    R_average = R_average / (src_image.n_rows * src_image.n_cols);
    G_average = G_average / (src_image.n_rows * src_image.n_cols);
    B_average = B_average / (src_image.n_rows * src_image.n_cols);
    long double S = (R_average + G_average + B_average) / 3;
 
    for (uint str  = 0; str < src_image.n_rows; ++str){
        for (uint col = 0; col < src_image.n_cols; ++col) {
            std::get<0>(src_image(str, col)) *= S / R_average;
            std::get<1>(src_image(str, col)) *= S / G_average;
            std::get<2>(src_image(str, col)) *= S / B_average;

        }
    }
    return src_image;
}

Image resize(Image src_image, double scale) {
    return src_image;
}

Image custom(Image src_image, Matrix<double> kernel) {
    // Function custom is useful for making concrete linear filtrations
    // like gaussian or sobel. So, we assume that you implement custom
    // and then implement other filtrations using this function.
    // sobel_x and sobel_y are given as an example.
    return src_image;
}



Image autocontrast(Image src_image, double fraction) {

    int y[256] = {};
    long double r_new, g_new, b_new;
    int r = 0, g = 0, b = 0;
    for (uint i = 0; i < src_image.n_rows; i++) {
        for (uint j = 0; j < src_image.n_cols; j++) {
            std::tie(r, g, b) = src_image(i,j);
            int cor = int(0.2125 * r + 0.7154 * g + 0.0721 * b);
            y[cor]++;
        }
    }

    int delta = int(fraction * src_image.n_rows * src_image.n_cols);
    int y_min = 0, k = 0;

    while (delta >= 0) {
        delta -= y[k];
        k++;
    }
    y_min = k - 1;
 
    delta = int(fraction * src_image.n_rows * src_image.n_cols);

    int y_max = 255;
    k = 255;
    while (delta >= 0) {
        delta -= y[k];
        k--;
    }
    y_max = k + 1;

    long double coef = 0;
    for (uint str = 0; str < src_image.n_rows; ++str) {
        for (uint col = 0; col < src_image.n_cols; ++col) {
            std::tie(r, g, b) = src_image(str, col);
            double cor = 0.2125 * r + 0.7154 * g + 0.0721 * b;
            coef = (cor - y_min) * (255.0 / (y_max - y_min)) / cor;

            r_new = coef * r;
            g_new = coef * g;
            b_new = coef * b;
            src_image(str, col) = check(r_new, g_new, b_new);
        }
    }
    return src_image;
}

Image gaussian(Image src_image, double sigma, int radius)  {
    return src_image;
}

Image gaussian_separable(Image src_image, double sigma, int radius) {
    return src_image;
}

std::tuple<uint, uint, uint> Seasrh_Median (Image img, uint size)
{
    int * array_r = new int [256];
    int * array_g = new int [256];
    int * array_b = new int [256];
    memset(array_r, 0, 256);
    memset(array_g, 0, 256);
    memset(array_b, 0, 256);
    uint r, g, b;
    uint num_r = 0;
    uint num_g = 0;
    uint num_b = 0;

    for (uint str = 0; str < size; ++str) {
        for (uint col = 0; col < size; ++col) {
            std::tie(r, g, b) = img(str, col);
            ++array_r[r];
            ++array_g[g];
            ++array_b[b];
        }
    }
    uint r_res = 0, g_res = 0, b_res = 0;

    for (uint i = 0; i < 256; ++i) {
        num_r += array_r[i];
        if (num_r >= ((size * size - 1) / 2 + 1)){
            r_res = i;
            break;
        }
    }

    for (uint i = 0; i < 256; ++i) {
        num_g += array_g[i];
        if (num_g >= ((size * size - 1) / 2 + 1)){
            g_res = i;
            break;
        }
    }

    for (uint i = 0; i < 256; ++i) {
        num_b += array_b[i];
        if (num_b >= ((size * size - 1) / 2 + 1)){
            b_res = i;
            break;
        }
    }

    return std::make_tuple(r_res, g_res, b_res);

    delete [] array_r;
    delete [] array_g;
    delete [] array_b;

}

class MedianFilterOp
{
public:
    std::tuple<uint, uint, uint> operator () (const Image &m) const
    {
        uint size = 2 * radius + 1;
        return Seasrh_Median(m, size);
    }
    static const int radius = 2;
};

Image median(Image src_image, int radius) {
    Image img = Mirror_Add_Frame(src_image, 15);
    Image res = img.unary_map(MedianFilterOp());
    src_image = Mirror_Drop_Frame(res, 15);
    return src_image;
}

Image median_linear(Image src_image, int radius) {
    return src_image;
}

Image median_const(Image src_image, int radius) {
    return src_image;
}

Image canny(Image src_image, int threshold1, int threshold2) {
    return src_image;
}
