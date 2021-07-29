#ifndef SLIC_H
#define SLIC_H

/* slic.h.
 *
 * Written by: Pascal Mettes.
 *
 * This file contains the class elements of the class Slic. This class is an
 * implementation of the SLIC Superpixel algorithm by Achanta et al. [PAMI'12,
 * vol. 34, num. 11, pp. 2274-2282].
 *
 * This implementation is created for the specific purpose of creating
 * over-segmentations in an OpenCV-based environment.
 */

#include <stdio.h>
#include <math.h>
#include <vector>
#include <float.h>

using namespace std;

#include <opencv2/opencv.hpp>

#include <vector>
#include <iostream>
#include <Core/Engine.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/types_c.h>


using namespace cv;

/* 2d matrices are handled by 2d vectors. */
#define vec2dd vector<vector<double> >
#define vec2di vector<vector<int> >
#define vec2db vector<vector<bool> >
/* The number of iterations run by the clustering algorithm. */
#define NR_ITERATIONS 10

/*
 * class Slic.
 *
 * In this class, an over-segmentation is created of an image, provided by the
 * step-size (distance between initial cluster locations) and the colour
 * distance parameter.
 */


struct Superpixel
{
    glm::vec4 coord;
    glm::vec4 colors;
    glm::vec4 normals;
    glm::vec4 positions;
    glm::vec4 weight;
};


class Superpixels {
public:                              // base alignment               // aligned offset
    vector<cv::Vec4f> coord;        // 16  *  coord.size()          // 0  (coord[0])
                                    // 16  *  coord.size()          // coord.size() * 16
    vector<cv::Vec4f> colors;
    vector<cv::Vec4f> normals;
    vector<cv::Vec4f> positions;
    vector<float> weight;

};

//class Superpixels {
//public:
//    vector<cv::Vec2f> coord; 
//    vector<cv::Vec3f> colors;
//    vector<cv::Vec3f> normals;
//    vector<cv::Vec3f> positions;
//    vector<float> weight;
//
//};

class Slic {
private:
    /* The cluster assignments and distance values for each pixel. */
    vec2di clusters;
    vec2dd distances;

    /* The LAB and xy values of the centers. */
    vec2dd centers;
    /* The number of occurences of each center. */
    vector<int> center_counts;

    /* The step size per cluster, and the colour (nc) and distance (ns)
     * parameters. */
    int step, nc, ns;

    /* Compute the distance between a center and an individual pixel. */
    double compute_dist(int ci, cv::Point pixel, cv::Scalar colour);
    /* Find the pixel with the lowest gradient in a 3x3 surrounding. */
    cv::Point find_local_minimum(const cv::Mat& image, cv::Point center);

    /* Remove and initialize the 2d vectors. */
    void clear_data();
    void init_data(const cv::Mat& image);

public:
    /* Class constructors and deconstructors. */
    Slic();
    ~Slic();

    /* Generate an over-segmentation for an image. */
    void generate_superpixels(const cv::Mat& image, int step, int nc);
    /* Enforce connectivity for an image. */
    void create_connectivity(const cv::Mat& image);

    /* Draw functions. Resp. displayal of the centers and the contours. */
    void display_center_grid(cv::Mat& image, cv::Vec3b colour);
    void display_contours(cv::Mat& image, cv::Vec3b colour);
    void colour_with_cluster_means(cv::Mat& image);
    void grid_color(cv::Mat& image);
    vector<Superpixel> getSuperpixels(cv::Mat& image, cv::Mat& imageN, cv::Mat& imageP);
    int nr_superpixels;

};

#endif