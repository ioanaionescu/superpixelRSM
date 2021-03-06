#include "slic.h"
#include <iostream>
#include <fstream>


/*
 * Constructor. Nothing is done here.
 */
Slic::Slic() {

}

/*
 * Destructor. Clear any present data.
 */
Slic::~Slic() {
    clear_data();
}

/*
 * Clear the data as saved by the algorithm.
 *
 * Input : -
 * Output: -
 */
void Slic::clear_data() {
    clusters.clear();
    distances.clear();
    centers.clear();
    center_counts.clear();
}

/*
 * Initialize the cluster centers and initial values of the pixel-wise cluster
 * assignment and distance values.
 *
 * Input : The image (cv::Mat).
 * Output: -
 */
void Slic::init_data(const cv::Mat& image) {
    /* Initialize the cluster and distance matrices. */
    for (int i = 0; i < image.cols; i++) {
        vector<int> cr;
        vector<double> dr;
        for (int j = 0; j < image.rows; j++) {
            cr.push_back(-1);
            dr.push_back(FLT_MAX);
        }
        clusters.push_back(cr);
        distances.push_back(dr);
    }

    /* Initialize the centers and counters. */
    for (int i = step; i < image.cols - step / 2; i += step) {
        for (int j = step; j < image.rows - step / 2; j += step) {
            vector<double> center;
            /* Find the local minimum (gradient-wise). */
            cv::Point nc = find_local_minimum(image, cv::Point(i, j));
            cv::Vec3f clr = image.at<cv::Vec3f>(nc.y, nc.x);
            cv::Scalar colour(clr[0], clr[1], clr[2]);
            //std::cerr<<colour<<"\n";
            //cv::Scalar clr_test = image.at<cv::Scalar>(nc.y, nc.x);
            //std::cerr<<clr_test<<"\n";

            /* Generate the center vector. */
            center.push_back(colour.val[0]);
            center.push_back(colour.val[1]);
            center.push_back(colour.val[2]);
            center.push_back(nc.x);
            center.push_back(nc.y);

            /* Append to vector of centers. */
            centers.push_back(center);
            center_counts.push_back(0);
        }
    }
}

/*
 * Compute the distance between a cluster center and an individual pixel.
 *
 * Input : The cluster index (int), the pixel (cv::Point), and the Lab values of
 *         the pixel (cv::Scalar).
 * Output: The distance (double).
 */
double Slic::compute_dist(int ci, cv::Point pixel, cv::Scalar colour) {
    double dc = sqrt(pow(centers[ci][0] - colour.val[0], 2) + pow(centers[ci][1]
        - colour.val[1], 2) + pow(centers[ci][2] - colour.val[2], 2));
    double ds = sqrt(pow(centers[ci][3] - pixel.x, 2) + pow(centers[ci][4] - pixel.y, 2));

    return sqrt(pow(dc / nc, 2) + pow(ds / ns, 2));

    //double w = 1.0 / (pow(ns / nc, 2));
    //return sqrt(dc) + sqrt(ds * w);
}

/*
 * Find a local gradient minimum of a pixel in a 3x3 neighbourhood. This
 * method is called upon initialization of the cluster centers.
 *
 * Input : The image (cv::Mat &) and the pixel center (cv::Point).
 * Output: The local gradient minimum (cv::Point).
 */
cv::Point Slic::find_local_minimum(const cv::Mat& image, cv::Point center) {
    double min_grad = FLT_MAX;
    cv::Point loc_min = cv::Point(center.x, center.y);

    for (int i = center.x - 1; i < center.x + 2; i++) {
        for (int j = center.y - 1; j < center.y + 2; j++) {
            cv::Vec3f clr = image.at<Vec3f>(j + 1, i);
            cv::Scalar c1(clr[0], clr[1], clr[2]);
            clr = image.at<Vec3f>(j, i + 1);
            cv::Scalar c2(clr[0], clr[1], clr[2]);
            clr = image.at<Vec3f>(j, i);
            cv::Scalar c3(clr[0], clr[1], clr[2]);
            /* Convert colour values to grayscale values. */
            double i1 = c1.val[0];
            double i2 = c2.val[0];
            double i3 = c3.val[0];
            /*double i1 = c1.val[0] * 0.11 + c1.val[1] * 0.59 + c1.val[2] * 0.3;
            double i2 = c2.val[0] * 0.11 + c2.val[1] * 0.59 + c2.val[2] * 0.3;
            double i3 = c3.val[0] * 0.11 + c3.val[1] * 0.59 + c3.val[2] * 0.3;*/

            /* Compute horizontal and vertical gradients and keep track of the
               minimum. */
            if (sqrt(pow(i1 - i3, 2)) + sqrt(pow(i2 - i3, 2)) < min_grad) {
                min_grad = fabs(i1 - i3) + fabs(i2 - i3);
                loc_min.x = i;
                loc_min.y = j;
            }
        }
    }

    return loc_min;
}

/*
 * Compute the over-segmentation based on the step-size and relative weighting
 * of the pixel and colour values.
 *
 * Input : The Lab image (IplImage*), the stepsize (int), and the weight (int).
 * Output: -
 */
void Slic::generate_superpixels(const cv::Mat& image, int step, int nc) {
    this->step = step;
    this->nc = nc;
    this->ns = step;

    /* Clear previous data (if any), and re-initialize it. */
    clear_data();
    init_data(image);

    /* Run EM for 10 iterations (as prescribed by the algorithm). */
    for (int i = 0; i < NR_ITERATIONS; i++) {
        /* Reset distance values. */
        for (int j = 0; j < image.cols; j++) {
            for (int k = 0; k < image.rows; k++) {
                distances[j][k] = FLT_MAX;
            }
        }

        for (int j = 0; j < (int)centers.size(); j++) {
            /* Only compare to pixels in a 2 x step by 2 x step region. */
            for (int k = int(centers[j][3]) - step; k < int(centers[j][3]) + step; k++) {
                for (int l = int(centers[j][4]) - step; l < int(centers[j][4]) + step; l++) {

                    if (k >= 0 && k < image.cols && l >= 0 && l < image.rows) {
                        cv::Vec3f clr = image.at<cv::Vec3f>(l, k);
                        cv::Scalar colour(clr[0], clr[1], clr[2]);
                        double d = compute_dist(j, cv::Point(k, l), colour);

                        /* Update cluster allocation if the cluster minimizes the
                           distance. */
                        if (d < distances[k][l]) {
                            distances[k][l] = d;
                            clusters[k][l] = j;
                        }
                    }
                }
            }
        }

        /* Clear the center values. */
        for (int j = 0; j < (int)centers.size(); j++) {
            centers[j][0] = centers[j][1] = centers[j][2] = centers[j][3] = centers[j][4] = 0;
            center_counts[j] = 0;
        }

        /* Compute the new cluster centers. */
        for (int j = 0; j < image.cols; j++) {
            for (int k = 0; k < image.rows; k++) {
                int c_id = clusters[j][k];

                if (c_id != -1) {
                    cv::Vec3f clr = image.at<cv::Vec3f>(k, j);
                    cv::Scalar colour(clr[0], clr[1], clr[2]);

                    centers[c_id][0] += colour.val[0];
                    centers[c_id][1] += colour.val[1];
                    centers[c_id][2] += colour.val[2];
                    centers[c_id][3] += j;
                    centers[c_id][4] += k;

                    center_counts[c_id] += 1;
                }
            }
        }

        /* Normalize the clusters. */
        for (int j = 0; j < (int)centers.size(); j++) {
            centers[j][0] /= center_counts[j];
            centers[j][1] /= center_counts[j];
            centers[j][2] /= center_counts[j];
            centers[j][3] /= center_counts[j];
            centers[j][4] /= center_counts[j];
        }
    }
}

/*
 * Enforce connectivity of the superpixels. This part is not actively discussed
 * in the paper, but forms an active part of the implementation of the authors
 * of the paper.
 *
 * Input : The image (cv::Mat).
 * Output: -
 */
void Slic::create_connectivity(const cv::Mat& image) {
    int label = 0, adjlabel = 0;
    const int lims = (image.cols * image.rows) / ((int)centers.size());

    const int dx4[4] = { -1,  0,  1,  0 };
    const int dy4[4] = { 0, -1,  0,  1 };

    /* Initialize the new cluster matrix. */
    vec2di new_clusters;
    for (int i = 0; i < image.cols; i++) {
        vector<int> nc;
        for (int j = 0; j < image.rows; j++) {
            nc.push_back(-1);
        }
        new_clusters.push_back(nc);
    }

    for (int i = 0; i < image.cols; i++) {
        for (int j = 0; j < image.rows; j++) {
            if (new_clusters[i][j] == -1) {
                vector<cv::Point> elements;
                elements.push_back(cv::Point(i, j));

                /* Find an adjacent label, for possible use later. */
                for (int k = 0; k < 4; k++) {
                    int x = elements[0].x + dx4[k], y = elements[0].y + dy4[k];

                    if (x >= 0 && x < image.cols && y >= 0 && y < image.rows) {
                        if (new_clusters[x][y] >= 0) {
                            adjlabel = new_clusters[x][y];
                        }
                    }
                }

                int count = 1;
                for (int c = 0; c < count; c++) {
                    for (int k = 0; k < 4; k++) {
                        int x = elements[c].x + dx4[k], y = elements[c].y + dy4[k];

                        if (x >= 0 && x < image.cols && y >= 0 && y < image.rows) {
                            if (new_clusters[x][y] == -1 && clusters[i][j] == clusters[x][y]) {
                                elements.push_back(cv::Point(x, y));
                                new_clusters[x][y] = label;
                                count += 1;
                            }
                        }
                    }
                }

                /* Use the earlier found adjacent label if a segment size is
                   smaller than a limit. */
                if (count <= lims >> 2) {
                    for (int c = 0; c < count; c++) {
                        new_clusters[elements[c].x][elements[c].y] = adjlabel;
                    }
                    label -= 1;
                }
                label += 1;
            }
        }
    }
}

/*
 * Display the cluster centers.
 *
 * Input : The image to display upon (IplImage*) and the colour (cv::Scalar).
 * Output: -
 */
void Slic::display_center_grid(cv::Mat& image, cv::Vec3b colour) {
    for (int i = 0; i < (int)centers.size(); i++) {
        cv::circle(image, cv::Point2d(centers[i][3], centers[i][4]), 2, cv::Scalar(colour[0], colour[1], colour[2]), 2);
    }
}

/*
 * Display a single pixel wide contour around the clusters.
 *
 * Input : The target image (cv::Mat) and contour colour (cv::Scalar).
 * Output: -
 */
void Slic::display_contours(cv::Mat& image, cv::Vec3b colour) {
    const int dx8[8] = { -1, -1,  0,  1, 1, 1, 0, -1 };
    const int dy8[8] = { 0, -1, -1, -1, 0, 1, 1,  1 };

    /* Initialize the contour vector and the matrix detailing whether a pixel
     * is already taken to be a contour. */
    vector<cv::Point> contours;
    vec2db istaken;
    for (int i = 0; i < image.cols; i++) {
        vector<bool> nb;
        for (int j = 0; j < image.rows; j++) {
            nb.push_back(false);
        }
        istaken.push_back(nb);
    }

    /* Go through all the pixels. */
    for (int i = 0; i < image.cols; i++) {
        for (int j = 0; j < image.rows; j++) {
            int nr_p = 0;

            /* Compare the pixel to its 8 neighbours. */
            for (int k = 0; k < 8; k++) {
                int x = i + dx8[k], y = j + dy8[k];

                if (x >= 0 && x < image.cols && y >= 0 && y < image.rows) {
                    if (istaken[x][y] == false && clusters[i][j] != clusters[x][y]) {
                        nr_p += 1;
                    }
                }
            }

            /* Add the pixel to the contour list if desired. */
            if (nr_p >= 2) {
                contours.push_back(cv::Point(i, j));
                istaken[i][j] = true;
            }
        }
    }

    /* Draw the contour pixels. */
    for (int i = 0; i < (int)contours.size(); i++) {
        image.at<Vec3f>(contours[i].y, contours[i].x) = colour;
    }
}

/*
 * Give the pixels of each cluster the same colour values. The specified colour
 * is the mean RGB colour per cluster.
 *
 * Input : The target image (cv::Mat).
 * Output: -
 */
void Slic::colour_with_cluster_means(cv::Mat& image) {
    vector<cv::Vec3f> colours(centers.size());

    /* Gather the colour values per cluster. */
    for (int i = 0; i < image.cols; i++) {
        for (int j = 0; j < image.rows; j++) {
            int index = clusters[i][j];
            colours[index] += image.at<Vec3f>(j, i);
        }
    }


    /* Divide by the number of pixels per cluster to get the mean colour. */
    for (int i = 0; i < (int)colours.size(); i++) {
        //cout << colours[i] << "  " << center_counts[i]<< endl;
        colours[i] /= center_counts[i];
        
    }

    /* Fill in. */
    for (int i = 0; i < image.cols; i++) {
        for (int j = 0; j < image.rows; j++) {
            image.at<Vec3f>(j, i) = colours[clusters[i][j]];;
        }
    }

}


void Slic::grid_color(cv::Mat& image) {
    vector<cv::Vec3i> colours(centers.size());
    ofstream myfile;
    myfile.open("superpixels.txt");

    /* Gather the colour values per cluster. */
    for (int i = 0; i < image.cols; i++) {
        for (int j = 0; j < image.rows; j++) {
            int index = clusters[i][j];
            colours[index] += image.at<Vec3f>(j, i);
        }
    }


    /* Divide by the number of pixels per cluster to get the mean colour. */
    for (int i = 0; i < (int)colours.size(); i++) {
        //cout << colours[i] << "  " << center_counts[i]<< endl;
        colours[i] /= center_counts[i];

    }

    /* Fill in. */
    for (int i = 0; i < image.cols; i++) {
        for (int j = 0; j < image.rows; j++) {
            image.at<Vec3f>(j, i) = cv::Vec3f(0.0f);//colours[clusters[i][j]];;
        }
    }

    for (int i = 0; i < (int)centers.size(); i++) {
        // x, y, B, G, R
        myfile << centers[i][3] << " " << centers[i][4] << " " << colours[i][0] << " " << colours[i][1] << " " << colours[i][2] <<" " << center_counts[i]<< endl;
        cv::circle(image, cv::Point2d(centers[i][3], centers[i][4]), 2, cv::Scalar(colours[i][0], colours[i][1], colours[i][2]), 2);
    }

    myfile.close();

}


vector<Superpixel> Slic::getSuperpixels(cv::Mat& image, cv::Mat& imageN, cv::Mat& imageP) {
    vector<Superpixel> superpixels;

    vector<cv::Vec3f> colours(centers.size());
    vector<cv::Vec3f> coloursN(centers.size());
    vector<cv::Vec3f> coloursP(centers.size());
    ofstream myfile;
    myfile.open("superpixels.txt");

    /* Gather the colour values per cluster. */
    for (int i = 0; i < image.cols; i++) {
        for (int j = 0; j < image.rows; j++) {
            int index = clusters[i][j];
            colours[index] += image.at<Vec3f>(j, i);
            coloursN[index] += imageN.at<Vec3f>(j, i);
            coloursP[index] += imageP.at<Vec3f>(j, i);
        }
    }


    /* Divide by the number of pixels per cluster to get the mean colour. */
    for (int i = 0; i < (int)colours.size(); i++) {
        //cout << colours[i] << "  " << center_counts[i]<< endl;
        colours[i] /= center_counts[i];
        coloursN[i] /= center_counts[i];
        coloursP[i] /= center_counts[i];

    }

    /* Fill in. */
    for (int i = 0; i < image.cols; i++) {
        for (int j = 0; j < image.rows; j++) {
            image.at<Vec3f>(j, i) = cv::Vec3f(0.0f);//colours[clusters[i][j]];;
            imageN.at<Vec3f>(j, i) = cv::Vec3f(0.0f);
            imageP.at<Vec3f>(j, i) = cv::Vec3f(0.0f);
        }
    }
    int total_pixeli = 0;
    int total_superpixels = 0;
    for (int i = 0; i < (int)centers.size(); i++) {
        // x, y, B, G, R
        // scos pixelii negri
       // if (cv::Vec3f(colours[i][0], colours[i][1], colours[i][2]) != cv::Vec3f(0.0f))
        {
            nr_superpixels++;
            myfile << centers[i][3] << " " << centers[i][4]; // x y
            myfile << " " << colours[i][2] << " " << colours[i][1] << " " << colours[i][0];// R G B colors
            myfile << " " << coloursN[i][2] << " " << coloursN[i][1] << " " << coloursN[i][0];// R G B normals
            myfile << " " << coloursP[i][2] << " " << coloursP[i][1] << " " << coloursP[i][0];// R G B positions
            myfile << " " << center_counts[i]; // pondere
            myfile << endl;
            total_pixeli += center_counts[i];

            //superpixels.coord.push_back(cv::Vec2f(centers[i][3], centers[i][4]));
            //superpixels.colors.push_back(cv::Vec3f(colours[i][2], colours[i][1], colours[i][0])); // R G B
            //superpixels.normals.push_back(cv::Vec3f(coloursN[i][2], coloursN[i][1], coloursN[i][0])); // R G B
            //superpixels.positions.push_back(cv::Vec3f(coloursP[i][2], coloursP[i][1], coloursP[i][0])); // R G B
            //superpixels.weight.push_back(center_counts[i]);
            Superpixel superpixel;
            superpixel.coord = (glm::vec4(centers[i][3], centers[i][4], 0, 0));
            superpixel.colors = glm::vec4(colours[i][2], colours[i][1], colours[i][0], 0.0f); // R G B
            superpixel.normals = glm::vec4(coloursN[i][2], coloursN[i][1], coloursN[i][0], 0.0f); // R G B
            superpixel.positions = glm::vec4(coloursP[i][2], coloursP[i][1], coloursP[i][0], 0.0f); // R G B
            superpixel.weight = (glm::vec4(center_counts[i], 0, 0, 0));

            superpixels.push_back(superpixel);


            cv::circle(image, cv::Point2d(centers[i][3], centers[i][4]), 4, cv::Scalar(colours[i][0], colours[i][1], colours[i][2]), 7);
            cv::circle(imageN, cv::Point2d(centers[i][3], centers[i][4]), 4, cv::Scalar(coloursN[i][0], coloursN[i][1], coloursN[i][2]), 7);
            cv::circle(imageP, cv::Point2d(centers[i][3], centers[i][4]), 4, cv::Scalar(coloursP[i][0], coloursP[i][1], coloursP[i][2]), 7);
        }
        
    }
    myfile <<total_pixeli;
    for (int i = 0; i < nr_superpixels; i++) {
        superpixels[i].weight /= total_pixeli;
    }
    myfile.close();
    
    return superpixels;
}