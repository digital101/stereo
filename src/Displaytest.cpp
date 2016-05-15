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
int lowThreshold = 40;
int edgeThreshold;
int const max_lowThreshold = 100;
int ratio = 3;
int kernel_size = 3;
char* window_name = "Edge Map";


char* getImageType(int number)
{
    // find type
    int imgTypeInt = number%8;
    char* imgTypeString;

    switch (imgTypeInt)
    {
        case 0:
            imgTypeString = "8U";
            break;
        case 1:
            imgTypeString = "8S";
            break;
        case 2:
            imgTypeString = "16U";
            break;
        case 3:
            imgTypeString = "16S";
            break;
        case 4:
            imgTypeString = "32S";
            break;
        case 5:
            imgTypeString = "32F";
            break;
        case 6:
            imgTypeString = "64F";
            break;
        default:
            break;
    }

    // find channel
    int channel = (number/8) + 1;


    return imgTypeString;
}
int main(int argc, char **argv)
{

	std::cout << "device :  " << argv[1] << endl;
//	char dev_name[64] = "/dev/video1";
	VideoDevice videoDevice(argv[1]);

    videoDevice.open_device();
    videoDevice.init_device();
    videoDevice.start_capturing();


    Mat frame(ACTUAL_ROWS, ACTUAL_COLS, CV_8UC1);
    Mat frameSmoothed(ACTUAL_ROWS, ACTUAL_COLS, CV_8UC1);
    Mat dst;

    /// Create a window
    namedWindow( window_name, WINDOW_AUTOSIZE );

    /// Create a Trackbar for user to enter threshold
 //   createTrackbar( "Min Threshold:", window_name, &lowThreshold, max_lowThreshold, NULL );
    /// Create a Trackbar for user to enter edge threshold


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

        createTrackbar( "Edge Threshold:", window_name, &edgeThreshold, 256, NULL );

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
//       blur( frame, detected_edges, Size(3,3) );
       int d = 8;
       double sigmaColor = 90;
       double sigmaSpace = 90;
       bilateralFilter(frame, detected_edges, d, sigmaColor, sigmaSpace, BORDER_DEFAULT );

       /// Canny detector
       Canny( detected_edges, detected_edges, lowThreshold, lowThreshold*ratio, kernel_size );

       /// Using Canny's output as a mask, we display our result
       dst = Scalar::all(0);

       frame.copyTo( dst, detected_edges);





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
	   int edgeNumRhs = 0;
       int edgeNumLhs = 0;
       int rowEdge = 200;
       int rowOffSetRhs =  -60;
       Mat templ;
       int templStartX;
       int templStartY;
       int templWidth=20;
       int templHeight=20;
       int ROIWidth = 130;
       int ROIHeight = 90;
       Point matchLoc;
#define NUM_RHS_EDGES_TO_SEARCH 5
#define NUM_LHS_EDGES_TO_SEARCH 1
       int edgeStoreLhs[NUM_LHS_EDGES_TO_SEARCH];
       int edgeStoreRhs[NUM_RHS_EDGES_TO_SEARCH];

       int colEdgeLhsSearchStart = (ROIWidth/2)+10;
       int colEdgeLhsSearchEnd = (dst.cols - 1)/2;
       int colEdgeLhsSearchInc = 1;

       int rowEdgeRhs = rowEdge + rowOffSetRhs;


       for(int colEdgeLhs = colEdgeLhsSearchStart; colEdgeLhs < colEdgeLhsSearchEnd; colEdgeLhs += colEdgeLhsSearchInc)  //avoid grabbing a ROI outside the image
       {
    	   Mat result;
           double minEdgeVal = 255;

    	   if(edgeNumLhs < NUM_LHS_EDGES_TO_SEARCH)
    	   {
      		   if(dst.at<uchar>(rowEdge,colEdgeLhs) > edgeThreshold )
    		   {
      			   edgeStoreLhs[edgeNumLhs] = colEdgeLhs;
    			   //circle( dst, Point( colEdgeLhs, rowEdge ), 10,  Scalar(200), 2, 8, 0 );

    			   //Grab the template around the edge(only a pointer to the source image LHS
    			   templ = frame(	cv::Range(rowEdge 	 - (templWidth/2),  rowEdge   + (templWidth/2)),	//rows
    					   	   	    cv::Range(colEdgeLhs - (templHeight/2), colEdgeLhs + (templHeight/2)));  //cols

    			   imshow( "Template", templ );

    			   //Now we have an edge on the LHS, search the RHS

//    			   int colEdgeRhs=(dst.cols - 1)/2 + 100;

    			   //    		       for(int colEdgeRhs=((dst.cols - 1)/2) + 10; colEdgeRhs < ((dst.cols - 1 - (ROIWidth/2))); colEdgeRhs++)
//    		       int colEdgeRhsSearchStart = (ROIWidth/2)+10;
//    		       int colEdgeRhsSearchEnd = (((dst.cols - 1)/2) - (ROIWidth/2));
    		       int colEdgeRhsSearchStart = (dst.cols - 1)/2;
    		       int colEdgeRhsSearchEnd = ((dst.cols - 1) - (ROIWidth/2));
    		       int colEdgeRhsSearchInc = 1;


    		       for(int colEdgeRhs=colEdgeRhsSearchStart; colEdgeRhs < colEdgeRhsSearchEnd; colEdgeRhs += colEdgeRhsSearchInc)
    			   {

    		    	   if(edgeNumRhs < NUM_RHS_EDGES_TO_SEARCH)
    		    	   {
    		      		   if(dst.at<uchar>(rowEdgeRhs, colEdgeRhs) > edgeThreshold )
    		    		   {

    		      			   Mat ROI;

        		    	       {
    		      				   char textBuf[64];
     //   		    	           sprintf (&textBuf[0], "Type =  %s, x = %ld, y = %ld", getImageType(dst.type()), dst.cols, dst.rows);
       		    	               sprintf (	&textBuf[0],
       		    	            		   	   	"#%ld, dst.at<uchar> = %d, colEdgeRhs = %ld",
       		    	            		   	    edgeNumRhs,
       		    	            		   	    dst.at<uchar>(rowEdgeRhs, colEdgeRhs),
       		    	            		   	    colEdgeRhs);

        		    	           putText(	dst,
        		    	        		   	textBuf,
        		    	      		   	    Point2f(100, (100 + edgeNumRhs*20)),
        		    	        		   	FONT_HERSHEY_PLAIN,
        		    	                    2,
        		    	                    Scalar::all(200));
        		    	       }

//    		    			   circle( dst, Point( colEdgeRhs, rowEdge  + (rowOffSetRhs) ), 10,  Scalar(200), 2, 8, 0 );

    		    			   //Grab the ROI around the edge only a pointer to the RHS source image in which to search
    		    			   //As the calibration impoves this ROI can be reduced.
    		    			   ROI = frame(	cv::Range(rowEdgeRhs - (ROIHeight/2),  rowEdgeRhs + (ROIHeight/2)),	 //rows
    		    					   	   	cv::Range(colEdgeRhs - (ROIWidth/2),   colEdgeRhs + (ROIWidth/2)));  //cols

    		    			   edgeStoreRhs[edgeNumRhs] = colEdgeRhs;
//#ifdef _REMOVE

    		    			   // Create the result matrix (need to improve efficiency here in creating this mat obj
    		    	           int result_cols =  ROI.cols - templ.cols + 1;
    		    	           int result_rows =  ROI.rows - templ.rows + 1;

    		    	           result.create( result_cols, result_rows, CV_32FC1 );

    		    			   //Match with our LHS template
    		    		       matchTemplate(ROI, templ, result, CV_TM_SQDIFF);
    		    		       normalize( result, result, 0, 1, NORM_MINMAX, -1, Mat() );
    		    			   imshow( "matchnorm", result );

    		    		       /// Localizing the best match with minMaxLoc
    		    		       double minVal; double maxVal; Point minLoc; Point maxLoc;

    		    		       minMaxLoc( result, &minVal, &maxVal, &minLoc, &maxLoc , Mat());

    		    		       if( minVal < minEdgeVal)
    		    		       {
               		    	        {
            		      				   char textBuf[64];
             //   		    	           sprintf (&textBuf[0], "Type =  %s, x = %ld, y = %ld", getImageType(dst.type()), dst.cols, dst.rows);
               		    	               sprintf (	&textBuf[0],
               		    	            		   	   	"minloc.x = %d, minloc.y = %d, minVal = %f",
               		    	            		   	    minLoc.x,
               		    	            		   	    minLoc.y,
               		    	            		   	    minVal);

                		    	           putText(	dst,
                		    	        		   	textBuf,
                		    	      		   	    Point2f(100, (130 + edgeNumRhs*20)),
                		    	        		   	FONT_HERSHEY_PLAIN,
                		    	                    2,
                		    	                    Scalar::all(200));
                		    	   }
   		    		    	       minEdgeVal = minVal;
    		    		    	   matchLoc = minLoc;
        		    		       //Fix location back into dst.
        		    		       matchLoc.x += (colEdgeRhs - (ROIWidth/2));
        		    		       //matchLoc.x -= (templ.cols/2);
        		    		       matchLoc.y += (rowEdgeRhs - (ROIHeight/2));
        		    		       //matchLoc.y -= (templ.rows/2);
    		    		       }
//#endif  //_REMOVE
   		    			       edgeNumRhs++;
   		    		    	   colEdgeRhs+=8;

    		    		   }
    		    	   }
    		       }
       			   edgeNumLhs++;
       	    	   colEdgeLhs+=8;

    		   }

    	   }
       }

       //draw the regions for the row  !!need to do this 2 dims for more than one row...
#ifndef _REMOVE
       rectangle( dst,
    		   	  matchLoc,
    		   	  Point( (matchLoc.x + (templ.cols)),
    		   			  matchLoc.y + (templ.rows) ),
    		   	  Scalar::all(200), 2, 8, 0 );
#endif //_REMOVE
       for(int edge = 0; edge <  edgeNumRhs ; edge++)
       {
		   {

			   rectangle(  dst,
					   	   Point( edgeStoreRhs[edge] - (ROIWidth/2), rowEdgeRhs   - (ROIHeight/2) ),
					   	   Point( edgeStoreRhs[edge] + (ROIWidth/2), rowEdgeRhs   + (ROIHeight/2) ),
	        		   	   Scalar::all(200), 2, 8, 0 );
		   }

       }
       for(int edge = 0; edge <  edgeNumLhs ; edge++)
       {
		   {

			   circle( dst, Point( edgeStoreLhs[edge], rowEdge ), 10,  Scalar(200), 2, 8, 0 );
		   }

       }

       imshow( window_name, dst );


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
