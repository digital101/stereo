#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/imgproc.hpp"
#include "videodevice.h"

#include <iostream>

using namespace cv;
using namespace std;

#define ACTUAL_ROWS 480
#define ACTUAL_COLS 1280

#define THRESH 180

//#define COLOURSPACE_YUV

int edgeThresh = 1;
int lowThreshold = 50;
int edgeThreshold;
int const max_lowThreshold = 100;
int ratio = 3;
int kernel_size = 3;
char* window_name = "Edge Map";


int main(int argc, char **argv)
{

	std::cout << "device :  " << argv[1] << endl;
//	char dev_name[64] = "/dev/video1";
	VideoDevice videoDevice(argv[1]);

    videoDevice.open_device();
    videoDevice.init_device();
    videoDevice.start_capturing();


    Mat frame(ACTUAL_ROWS, ACTUAL_COLS, CV_8U);
    Mat dst;

    /// Create a window
    namedWindow( window_name, WINDOW_AUTOSIZE );

    /// Create a Trackbar for user to enter threshold
 //   createTrackbar( "Min Threshold:", window_name, &lowThreshold, max_lowThreshold, NULL );
    /// Create a Trackbar for user to enter edge threshold
    createTrackbar( "Edge Threshold:", window_name, &edgeThreshold, 256, NULL );


    while( 1 /*cap.isOpened()*/ )   // check if we succeeded
    {

        Mat greyframe;
        Mat harrisFrame;
        Mat edgeFrame;
        Mat detected_edges;
        Mat dst_norm;
        Mat dst_norm_scaled;
        void * videobuffer;
        size_t size;
        long frame_count = 0;


        videoDevice.get_frame(&videobuffer, &size, &frame_count);
        if(size == 0 )
        {
        	cout << "size == 0" << endl;
        	return -1;
        }

        if(videobuffer == NULL)
        {
        	cout << "videobuffer == NULL"  << endl;
        	return -1;
        }


#ifdef COLOURSPACE_YUV

#else
	   char* videoBufPtr = (char*)videobuffer;
       for(int row = 0; row < ACTUAL_ROWS; row++)
       {
    	   for(int col = 0; col < ACTUAL_COLS; col++)
           {
    		   int rowInIndex = row * ACTUAL_COLS;
    		   int colInIndex = col;

    		   frame.at<uchar>(row, col) = videoBufPtr[rowInIndex + colInIndex];
           }
       }

#endif   //COLOURSPACE_YUV


#ifndef BYPASS_PROC


#ifndef BYPASS_CANNY

       /// Reduce noise with a kernel 3x3
       blur( frame, detected_edges, Size(3,3) );

       /// Canny detector
       Canny( detected_edges, detected_edges, lowThreshold, lowThreshold*ratio, kernel_size );

       /// Using Canny's output as a mask, we display our result
       dst = Scalar::all(0);

       frame.copyTo( dst, detected_edges);

//       {
//    	   char textBuf[64];

//           sprintf (&textBuf[0], "num of frames %ld",frame_count);

//           putText(	frame,
//        		   	textBuf,
//       		   	Point2f(20,20),
//        		   	FONT_HERSHEY_PLAIN,
//                    2,
//                    Scalar(0,0,255,255));
//       }



#endif //BYPASS_CANNY



#ifdef BYPASS_SOBEL
        //Read in a as 16 bit YUYV value and converted to RGB
        //In fact it is 2 mono pixels a byte each
//        cv::cvtColor(frame, greyframe, CV_BGR2GRAY);
        //Sobel(InputArray src, OutputArray dst, int ddepth, int dx, int dy, int ksize=3, double scale=1, double delta=0, int borderType=BORDER_DEFAULT )
        Sobel(	frame,
        		edgeFrame,
        		CV_8U, //depth
        		1,// dx,
        		1,//int dy,
        		3, //ksize=3,
        		1, //double scale=1,
        		0, //double delta=0,
        		BORDER_DEFAULT );

#endif //BYPASS_SOBEL

#ifdef BYPASS_HARRIS

        cv::cornerHarris(frame, harrisFrame, 3, 3, 0.06, BORDER_DEFAULT );


        cv::normalize( harrisFrame, dst_norm, 0, 255, NORM_MINMAX, CV_32FC1, Mat() );
        cv::convertScaleAbs( dst_norm, dst_norm_scaled );

        int featurecount = 0;
        for( int j = 0; j < dst_norm.rows ; j++ )
           { for( int i = 0; i < dst_norm.cols; i++ )
                {
                  if( (int) dst_norm.at<float>(j,i) > THRESH )
                    {
                      if(featurecount < 20)
                      {
                    	  //circle( dst_norm_scaled, Point( i, j ), 10,  Scalar(0), 2, 8, 0 );
                    	  circle( edgeFrame, Point( i, j ), 10,  Scalar(200), 2, 8, 0 );
                    	  featurecount ++;
                      }
                    }
                }
           }


//        cv::namedWindow( corners_window, WINDOW_AUTOSIZE );
        cv::imshow( "corners_window", edgeFrame );
#endif //BYPASS_HARRIS



       //Search for edge to get template to match
       int edgeMax=0;
       int rowEdge = 200;
       for(int i=0; i< (dst.cols - 1)/2; i++)
       {
#ifndef COMMENTALLOUT
    	   if(edgeMax < 1)
    	   {
      		   if(dst.at<uchar>(rowEdge,i) > edgeThreshold )
    		   {
    			   circle( dst, Point( i, rowEdge ), 10,  Scalar(200), 2, 8, 0 );
    			   edgeMax++;
    			   i+= 2;
    		   }
    	   }
#endif
       }
       imshow( window_name, dst );

       //matchTemplate(InputArray image, InputArray templ, OutputArray result, int method);

       imshow("lalala",frame);
#else //BYPASS_PROC
       imshow("lalala",frame);
#endif



       int k = waitKey(100);
        if ( k==27 )
            break;
       videoDevice.unget_frame();

    }

    while(1)
    {
    	int k = waitKey(100);
        if ( k==27 )
             break;
    }

//    videoDevice.stop_capturing();
//    videoDevice.uninit_device();

    return 0;
}
