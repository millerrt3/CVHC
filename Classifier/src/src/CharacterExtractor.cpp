#include "CharacterExtractor.h"

using namespace cv;
// use a few things from the std namespace for console I/O
using std::cout;
using std::cin;
using std::endl;

//specify some parameters that will be used for every processed image
string rootPath;
Size blur_kernel = Size(4, 4);
int erode_kernel_size = 2; //article says 4
double area;
Size size(32, 32);
int pad = 4;
int square_dimension, x, y, max_x, max_y, processed_count;
Size image_dim;

/*
Pre-Process an image
  1. Convert to grayscale
  2. Apply kernel smoothing to reduce noise
  3. Treshold to b/w image and invert
  4. Erode and or Dilate the image to reduce internal and external noise
should take the srcImg and apply all the processing, placing the result in dstImg. Because the thresholdImg is necessary
for the end product image, it is also passed in and populated, so that it may be used by the caller later.
*/
int CharacterExtractor::preprocessImage(Mat srcImg, Mat& dstImg, Mat& threshImg, bool shouldDilate){
  //convert it to grayscale
  Mat grayImg;
  cvtColor(srcImg, grayImg, CV_BGR2GRAY);

  //Kernel Smoothing: apply a blur to reduce noise using an 8x8 kernel
  Mat blurImg;
  blur(grayImg, blurImg, blur_kernel);

  //Threshold
  //convert to b / w image, and invert: ie: (black, white) = (0, 1)
  Mat thresholdImg;
  threshold(blurImg, thresholdImg, 200, 255, 1); //1 for inverted binary threshold type

  //Erosion:
  Mat erodedImg;
  Mat eroded_element = getStructuringElement(1,
    Size(2 * erode_kernel_size + 1, 2 * erode_kernel_size + 1),
    Point(erode_kernel_size, erode_kernel_size));
  erode(thresholdImg, erodedImg, eroded_element);
  
  if (!shouldDilate){
    threshImg = thresholdImg;
    dstImg = erodedImg;
    return 0;
  }

  //Dilation:
  Mat dilatedImg;
  int dilate_kernel_size = 2; //article says 2
  Mat dilated_element = getStructuringElement(1,
    Size(2 * dilate_kernel_size + 1, 2 * dilate_kernel_size + 1),
    Point(dilate_kernel_size, dilate_kernel_size));

  dilate(erodedImg, dilatedImg, dilated_element);

  threshImg = thresholdImg;
  dstImg = dilatedImg;
  return 0;
}

/*
findBoundingBoxes: takes an image and finds the boxes bounding the characters inside that image
*/
int CharacterExtractor::findBoundingBoxes(Mat srcImg, vector<Rect>& foundBoxes){

  //Contours, polygonal conversions and bouding boxes
  /// Find contours
  vector<vector<Point> > contours;
  vector<Vec4i> hierarchy;
  findContours(srcImg, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

  ////draw contours
  //RNG rng(12345);
  //Mat contour_image = Mat::zeros(eroded_image.size(), CV_8UC3);
  //for (int i = 0; i< contours.size(); i++){
  //  Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
  //  drawContours(contour_image, contours, i, color, 2, 8, hierarchy, 0, Point());
  //}

  // Approximate the contours as polygons, and define a bounding rectangle that holds each polygon
  vector<vector<Point> > contours_poly(contours.size());
  vector<Rect> boundRect(contours.size());
  
  for (int i = 0; i < contours.size(); i++){
    approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
    boundRect[i] = boundingRect(Mat(contours_poly[i]));
  }
  foundBoxes = boundRect;
  return 0;
}

/*
findFullCharBoxes: takes a vector of bounding boxes for an image and checks for/removes any 
boxes that might be internal bounding boxes (ie: a bounding box around the center hole in a p or d
*/
int CharacterExtractor::findFullCharBoxes(vector<Rect>& allBoxes, vector<Rect>& charBoxes){
  for (int i = 0; i < allBoxes.size(); i++){
    bool isInteriorBox = false;
    for (int j = 0; j < allBoxes.size(); j++){
      if (i != j){
        if (rectContainRect(allBoxes[j], allBoxes[i])){
          isInteriorBox = true;
        }
      }
    }
    if (!isInteriorBox){
      charBoxes.push_back(allBoxes[i]);
    }
  }
  return 0;
}

/*
rectContainsRect: checks if the rectangle called "container" completely contains the rectangle called "rect"
given the fixed orientation of the boxes, accomplish this by simply checking if the top left (tl) and bottom 
right (br) corner points of "rect" are contained by "container"
*/
bool CharacterExtractor::rectContainRect(Rect& container, Rect& rect){
  return container.contains(rect.tl()) && container.contains(rect.br());
}

/*
*/
int CharacterExtractor::cropImage(Mat srcImg, Mat& dstImg, Rect& charBox){

  square_dimension = 2 * pad + max(charBox.width, charBox.height);
  x = charBox.x - pad;
  y = charBox.y - pad;
  image_dim = srcImg.size();
  max_x = image_dim.width;
  max_y = image_dim.height;

  //check to make sure the padding doesn't try to select pixels outside the original dimension
  try{
    if ((x - pad <= 0) || (x + square_dimension + pad >= max_x) 
       || (y - pad <= 0) || (y + square_dimension + pad >= max_y)){
      Rect crop_region = charBox;
      Mat cropped_image = srcImg(crop_region); //crop the image using the largest bounding rectangle
      Mat resized_cropped_image;
      resize(cropped_image, resized_cropped_image, size);
      dstImg = resized_cropped_image;
      return 0;
    }
  }
  catch (int e){
    cout << "An exception occurred:" << e << endl;
    return -1;
  }
  try
  {
    Rect crop_region = Rect(x, y, square_dimension, square_dimension);
    Mat cropped_image = srcImg(crop_region); //crop the image using the largest bounding rectangle
    Mat resized_cropped_image;
    resize(cropped_image, resized_cropped_image, size);
    dstImg = resized_cropped_image;
    return 0;
  }
  catch (int e){
    cout << "An exception occurred:" << e << endl;
    return -1;
  }
  return 0;
}