#include <string>
#include <vector>
#include <fstream>
#include <cassert>
#include <iostream>
#include <cmath>

#include "classifier.h"
#include "EasyBMP.h"
#include "linear.h"
#include "argvparser.h"
#include "../include/matrix.h"
#include "../include/io.h"

using std::string;
using std::vector;
using std::ifstream;
using std::ofstream;
using std::pair;
using std::make_pair;
using std::cout;
using std::cerr;
using std::endl;
using std::get;

using CommandLineProcessing::ArgvParser;

typedef vector<pair<BMP*, int> > TDataSet;
typedef vector<pair<string, int> > TFileList;
typedef vector<pair<vector<float>, int> > TFeatures;

// Load list of files and its labels from 'data_file' and
// stores it in 'file_list'
void LoadFileList(const string& data_file, TFileList* file_list) {
    ifstream stream(data_file.c_str());

    string filename;
    int label;
    
    int char_idx = data_file.size() - 1;
    for (; char_idx >= 0; --char_idx)
        if (data_file[char_idx] == '/' || data_file[char_idx] == '\\')
            break;
    string data_path = data_file.substr(0,char_idx+1);
    
    while(!stream.eof() && !stream.fail()) {
        stream >> filename >> label;
        if (filename.size())
            file_list->push_back(make_pair(data_path + filename, label));
    }
    stream.close();
}

// Load images by list of files 'file_list' and store them in 'data_set'
void LoadImages(const TFileList& file_list, TDataSet* data_set) {
    //cout << file_list.size() << endl;
    for (size_t img_idx = 0; img_idx < file_list.size(); ++img_idx) {
            // Create image
        BMP* image = new BMP();
            // Read image from file
        image->ReadFromFile(file_list[img_idx].first.c_str());
            // Add image and it's label to dataset
        data_set->push_back(make_pair(image, file_list[img_idx].second));
    }
}

// Save result of prediction to file
void SavePredictions(const TFileList& file_list,
                     const TLabels& labels, 
                     const string& prediction_file) {
        // Check that list of files and list of labels has equal size 
    assert(file_list.size() == labels.size());
        // Open 'prediction_file' for writing
    ofstream stream(prediction_file.c_str());

        // Write file names and labels to stream
    for (size_t image_idx = 0; image_idx < file_list.size(); ++image_idx)
        stream << file_list[image_idx].first << " " << labels[image_idx] << endl;
    stream.close();
}

class SobelVertical
{
public:
    uint vert_radius = 1;
    uint hor_radius = 0;
    double operator () (const Matrix <double> &m) const {
        return (m(0, 0) - m(2, 0));
    }
};

class SobelHorizontal {
public:
    uint vert_radius = 0;
    uint hor_radius = 1;

    double operator()(const Matrix <double> &m) const {
        return (m(0, 2) - m(0, 0));
    }
};

class CalculateNeighborhoods{
public:
    uint vert_radius = 1;
    uint hor_radius = 1;

    uint operator ()(const Matrix <double> &m) const {
        uint tmp = 1 * (m(1, 1) <= m(0, 0)) +
                   2 * (m(1, 1) <= m(0, 1)) +
                   4 * (m(1, 1) <= m(0, 2)) +
                   8 * (m(1, 1) <= m(1, 0)) +
                   16 * (m(1, 1) <= m(1, 2)) +
                   32 * (m(1, 1) <= m(2, 0)) +
                   64 * (m(1, 1) <= m(2, 1)) +
                   128 * (m(1, 1) <= m(2, 2));
        return tmp;
    }
};

// Exatract features from dataset.
// You should implement this function by yourself =)
void ExtractFeatures(const TDataSet& data_set, TFeatures* features)
{
    for (size_t image_idx = 0; image_idx < data_set.size(); ++image_idx) {
        Matrix <double> m(data_set[image_idx].first->TellHeight(), data_set[image_idx].first->TellWidth());

        for (uint r = 0; r < m.n_rows; ++ r) {
            for (uint c = 0; c < m.n_cols; ++c) {
                //0.299R + 0.587G + 0.114B
                RGBApixel pixel = data_set[image_idx].first->GetPixel(c,r);
                m(r, c) = uint(0.299 * pixel.Red + 0.587 * pixel.Green + 0.114 * pixel.Blue);
            }
        }

        Matrix <double> vertical = m.unary_map(SobelVertical());
        Matrix <double> horizontal = m.unary_map(SobelHorizontal());
        Matrix <uint> neighbors = m.unary_map(CalculateNeighborhoods());

        Matrix <double> direction(vertical.n_rows, vertical.n_cols);
        Matrix <double> module(vertical.n_rows, vertical.n_cols);

        for (uint i = 0; i < vertical.n_rows; ++i) {
            for (uint j = 0; j < vertical.n_cols; ++j) {
                direction(i, j) = std::atan2(vertical(i, j), horizontal(i, j));
                module(i, j) = std::sqrt(vertical(i, j) * vertical(i, j) + horizontal(i, j) * horizontal(i, j));
            }
        }

        uint cnt_cell = 64;
        double segm_direction = (2 * M_PI) / 8;
        int segment [8][2];
        double tmp = -1 * M_PI;
        for (uint i = 0; i < 8; ++i) {
            segment[i][0] = tmp;
            segment[i][1] = tmp + segm_direction;
            tmp = tmp + segm_direction + 1;
        }
        vector <float> feat;
        uint cell_col = vertical.n_cols / 8;
        uint cell_row = vertical.n_rows / 8;
        uint cur_row = 0;
        uint cur_col = 0;
        int cnt_pixel_in_cell = cell_col * cell_row;
//        cout << "size" << vertical.n_rows << "\t" << vertical.n_cols << endl;
        for (uint cur_cell = 1; cur_cell < cnt_cell + 1; ++cur_cell) {
            double cur_hist[] = {0, 0, 0, 0, 0, 0, 0, 0};
            double templates_hist[256]= {0};
            double color_feat[3] = {0};
            double red_average = 0;
            double green_average = 0;
            double blue_average = 0;
//            cout << cur_row << '\t' << cur_col << endl;
            for (uint row = cur_row; row < (cur_row + cell_row); ++row) {
                for (uint col = cur_col; col < (cur_col + cell_col); ++col) {
                    RGBApixel pixel = data_set[image_idx].first->GetPixel(col,row);
                    red_average += pixel.Red;
                    green_average += pixel.Green;
                    blue_average += pixel.Blue;
                    for (uint i = 0; i < 8; ++i) {
                        if ((direction(row, col) <= segment[i][1]) and (direction(row, col) >= segment[i][0])) {
                            cur_hist[i] += module(row, col);
                            break;
                        }
                    }

                    templates_hist[neighbors(row, col)] += 1;
                }
            }

            red_average /= (cnt_pixel_in_cell * 255);
            green_average /= (cnt_pixel_in_cell * 255);
            blue_average /= (cnt_pixel_in_cell * 255);

            color_feat[0] = red_average;
            color_feat[1] = green_average;
            color_feat[2] = blue_average;

            double norm = 0;
            double template_norm = 0;
            for (uint i = 0; i < 8; ++i) {
                norm += cur_hist[i] * cur_hist[i];
            }

            for (int j = 0; j < 256; ++j) {
                template_norm += templates_hist[j] * templates_hist[j];
            }

            template_norm = std::sqrt(template_norm);
            norm = std::sqrt(norm);

            if (norm < 0.000000001) {
                norm = 1;
            }

            if (template_norm < 0.000000001) {
                template_norm = 1;
            }

            for (uint i = 0; i < 8; ++i) {
                cur_hist[i] /= norm;
                feat.push_back(float(cur_hist[i]));
            }

            for (int j = 0; j < 256; ++j) {
                templates_hist[j] /= template_norm;
            }

            feat.insert(feat.end(), std::begin(templates_hist), std::end(templates_hist));
            feat.insert(feat.end(), std::begin(color_feat), std::end(color_feat));

            cur_col = cur_col + cell_col;

            if (cur_cell % 8 == 0) {
                cur_row = cur_row + cell_row;
                cur_col = 0;
            }
        }
//        cout << feat.size() << endl;
        features->push_back(std::make_pair(feat, data_set[image_idx].second));
    }
}

// Clear dataset structure
void ClearDataset(TDataSet* data_set) {
        // Delete all images from dataset
    for (size_t image_idx = 0; image_idx < data_set->size(); ++image_idx)
        delete (*data_set)[image_idx].first;
        // Clear dataset
    data_set->clear();
}

// Train SVM classifier using data from 'data_file' and save trained model
// to 'model_file'
void TrainClassifier(const string& data_file, const string& model_file) {
        // List of image file names and its labels
    TFileList file_list;
        // Structure of images and its labels
    TDataSet data_set;
        // Structure of features of images and its labels
    TFeatures features;
        // Model which would be trained
    TModel model;
        // Parameters of classifier
    TClassifierParams params;
    
        // Load list of image file names and its labels
    LoadFileList(data_file, &file_list);
        // Load images
    LoadImages(file_list, &data_set);
        // Extract features from images
    ExtractFeatures(data_set, &features);

        // PLACE YOUR CODE HERE
        // You can change parameters of classifier here
    params.C = 0.01;
    TClassifier classifier(params);
        // Train classifier
    classifier.Train(features, &model);
        // Save model to file
    model.Save(model_file);
        // Clear dataset structure
    ClearDataset(&data_set);
}

// Predict data from 'data_file' using model from 'model_file' and
// save predictions to 'prediction_file'
void PredictData(const string& data_file,
                 const string& model_file,
                 const string& prediction_file) {
        // List of image file names and its labels
    TFileList file_list;
        // Structure of images and its labels
    TDataSet data_set;
        // Structure of features of images and its labels
    TFeatures features;
        // List of image labels
    TLabels labels;

        // Load list of image file names and its labels
    LoadFileList(data_file, &file_list);
        // Load images
    LoadImages(file_list, &data_set);
        // Extract features from images
    ExtractFeatures(data_set, &features);

        // Classifier 
    TClassifier classifier = TClassifier(TClassifierParams());
        // Trained model
    TModel model;
        // Load model from file
    model.Load(model_file);
        // Predict images by its features using 'model' and store predictions
        // to 'labels'
    classifier.Predict(features, model, &labels);

        // Save predictions
    SavePredictions(file_list, labels, prediction_file);
        // Clear dataset structure
    ClearDataset(&data_set);
}

int main(int argc, char** argv) {
    // Command line options parser
    ArgvParser cmd;
        // Description of program
    cmd.setIntroductoryDescription("Machine graphics course, task 2. CMC MSU, 2014.");
        // Add help option
    cmd.setHelpOption("h", "help", "Print this help message");
        // Add other options
    cmd.defineOption("data_set", "File with dataset",
        ArgvParser::OptionRequiresValue | ArgvParser::OptionRequired);
    cmd.defineOption("model", "Path to file to save or load model",
        ArgvParser::OptionRequiresValue | ArgvParser::OptionRequired);
    cmd.defineOption("predicted_labels", "Path to file to save prediction results",
        ArgvParser::OptionRequiresValue);
    cmd.defineOption("train", "Train classifier");
    cmd.defineOption("predict", "Predict dataset");
        
        // Add options aliases
    cmd.defineOptionAlternative("data_set", "d");
    cmd.defineOptionAlternative("model", "m");
    cmd.defineOptionAlternative("predicted_labels", "l");
    cmd.defineOptionAlternative("train", "t");
    cmd.defineOptionAlternative("predict", "p");

        // Parse options
    int result = cmd.parse(argc, argv);

        // Check for errors or help option
    if (result) {
        cout << cmd.parseErrorDescription(result) << endl;
        return result;
    }

        // Get values 
    string data_file = cmd.optionValue("data_set");
    string model_file = cmd.optionValue("model");
    bool train = cmd.foundOption("train");
    bool predict = cmd.foundOption("predict");

        // If we need to train classifier
    if (train)
        TrainClassifier(data_file, model_file);
        // If we need to predict data
    if (predict) {
            // You must declare file to save images
        if (!cmd.foundOption("predicted_labels")) {
            cerr << "Error! Option --predicted_labels not found!" << endl;
            return 1;
        }
            // File to save predictions
        string prediction_file = cmd.optionValue("predicted_labels");
            // Predict data
        PredictData(data_file, model_file, prediction_file);
    }
}