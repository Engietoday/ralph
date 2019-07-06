#include <memory>
#include <string>
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <sstream>

#include "util/ImageUtility.h"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/core.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/xfeatures2d.hpp"
#include "opencv2/ximgproc.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/calib3d.hpp"

#include <boost/thread.hpp>
#include <boost/chrono.hpp>

std::vector<std::string> split(std::string strToSplit, char delimeter)
{
	std::stringstream ss(strToSplit);
    std::string item;
    std::vector<std::string> splittedStrings;
    while (std::getline(ss, item, delimeter))
    {
       splittedStrings.push_back(item);
    }
    return splittedStrings;
}

int getdir(std::string dir, std::vector<std::string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        std::cout << "Error(" << errno << ") opening " << dir << std::endl;
        return errno;
    }
    while ((dirp = readdir(dp)) != NULL) {
        std::string temp_file = dirp->d_name;
        files.push_back(temp_file);
    }
    closedir(dp);
    return 0;
}


int getkittidir(std::string dir, std::vector<std::string> &files_out){
	std::vector<std::string> files;
	std::vector<int> file_number;
	getdir(dir, files);
	for(int i = 0; i < files.size(); i++){
		std::string current_file = files[i];
        	std::vector<std::string> current_file_split = split(current_file,'.');
		file_number.push_back(std::atoi(current_file_split[0].c_str()));	
	}
	int max = (int)(*max_element(file_number.begin(), file_number.end()));
	int min = (int)(*min_element(file_number.begin(), file_number.end()));
	for(int i = min; i <= max; i++){
		std::string temp_sorted_filename = std::to_string(i);
		std::string sorted_filename = std::to_string(i);
		for(int j = 0; j < (10 - temp_sorted_filename.length()); j++){
			sorted_filename = "0" + sorted_filename;
		}
		files_out.push_back(sorted_filename + ".png");	
	}
	return 0;
}

void print_loadingbar(float in_pct){
	// This function prints a simple status bar with 30 devisions.
	std::string status_bar = "                                                ";
	int indx = status_bar.length()*(in_pct/100);
	for(int i = 0; i <= indx; i++){
		status_bar.replace(i,1,"=");
	}
	std::cout << "PERCENT LOADED|" << status_bar <<  "|" << in_pct << "%" << std::endl;
}

std::vector<cv::Mat> getimages(std::string dir, std::vector<std::string> &files){
	std::vector<cv::Mat> vector_src;
	std::cout << "Loading images to memory. Verbose at every 50 images." << std::endl;
	for(int i = 0; i < files.size(); i++){
		std::string file_name = files[i];
		cv::Mat temp_Mat = cv::imread(dir + file_name);
		if(temp_Mat.empty()){
	        std::cout << "Could not open or find the image!" << std::endl;
	        break;
    	}else{
    		vector_src.push_back(temp_Mat);
    	}
		if(i%100 == 0){
			print_loadingbar((float)i/files.size()*100);
		}
	}
	return vector_src;
}



boost::mutex mtx_;

void wait(int seconds)
{
  boost::this_thread::sleep_for(boost::chrono::seconds{seconds});
}

void thread1()
{
  int id = 1;
  for (int i = 0; i < 10000; ++i)
  {
    wait(.5);
	//boost::lock_guard<boost::mutex> guard(mtx_);
	mtx_.lock();
    std::cout <<"ID: " << id << " VALUE: " << i << '\n';
	mtx_.unlock();
  }
}
void thread2()
{
  int id = 2;
  for (int i = 0; i < 10000; ++i)
  {
    wait(.5);
	//boost::lock_guard<boost::mutex> guard(mtx_);
	mtx_.lock();
    std::cout <<"\t\tID: " << id << " VALUE: " << i << '\n';
	mtx_.unlock();
  }
}

void playground_thread(){
	boost::thread_group tgroup;
	tgroup.create_thread(boost::bind(&thread1));
    tgroup.create_thread(boost::bind(&thread2));
 	tgroup.join_all();
}
void playground_featurepoint_detect(){
	std::string dir1 = std::string("/media/prismadynamics/Elements/KITTI/raw/2011_09_29/2011_09_29_drive_0071_sync/image_00/data/");
	std::vector<std::string> files = std::vector<std::string>();
	
	getkittidir(dir1, files);
	std::vector<cv::Mat> vector_src = getimages(dir1, files);
	int minHessian = 400;
    cv::Ptr<cv::xfeatures2d::SURF> detector = cv::xfeatures2d::SURF::create( minHessian );
    std::vector<cv::KeyPoint> keypoints;
    cv::Mat img_keypoints;
    cv::namedWindow("Display window", cv::WINDOW_NORMAL);// Create a window for display.
	for(int i = 0; i < vector_src.size(); i++){
		detector->detect( vector_src[i], keypoints );
        cv::drawKeypoints(vector_src[i], keypoints, img_keypoints, cv::Scalar(254) );
		cv::imshow("Display window",img_keypoints);
    	cv::waitKey(1000);     
	}
    cv::waitKey(0); 
}

using namespace cv;
using namespace cv::ximgproc;
using namespace std;
Rect computeROI(Size2i src_sz, Ptr<StereoMatcher> matcher_instance);
#define HAVE_EIGEN

const std::string keys =
    "{help h usage ? |                  | print this message                                                }"
    "{@left          |../data/aloeL.jpg | left view of the stereopair                                       }"
    "{@right         |../data/aloeR.jpg | right view of the stereopair                                      }"
    "{GT             |../data/aloeGT.png| optional ground-truth disparity (MPI-Sintel or Middlebury format) }"
    "{dst_path       |None              | optional path to save the resulting filtered disparity map        }"
    "{dst_raw_path   |None              | optional path to save raw disparity map before filtering          }"
    "{algorithm      |sgbm              | stereo matching method (bm or sgbm)                               }"
    "{filter         |fbs_conf          | used post-filtering (wls_conf or wls_no_conf or fbs_conf)         }"
    "{no-display     |                  | don't display results                                             }"
    "{no-downscale   |true              | force stereo matching on full-sized views to improve quality      }"
    "{dst_conf_path  |None              | optional path to save the confidence map used in filtering        }"
    "{vis_mult       |1.0               | coefficient used to scale disparity map visualizations            }"
    "{max_disparity  |160               | parameter of stereo matching                                      }"
    "{window_size    |-1                | parameter of stereo matching                                      }"
    "{wls_lambda     |500.0             | parameter of wls post-filtering                                   }"
    "{wls_sigma      |1.0               | parameter of wls post-filtering                                   }"
    "{fbs_spatial    |16.0              | parameter of fbs post-filtering                                   }"
    "{fbs_luma       |8.0               | parameter of fbs post-filtering                                   }"
    "{fbs_chroma     |8.0               | parameter of fbs post-filtering                                   }"
    "{fbs_lambda     |128.0             | parameter of fbs post-filtering                                   }"
    ;

int playground_stereo_matching(int argc, char** argv){
	std::string dir_left = std::string("/media/prismadynamics/Elements/KITTI/raw/2011_09_29/2011_09_26_drive_0051_sync/image_00/data/");
	std::string dir_right = std::string("/media/prismadynamics/Elements/KITTI/raw/2011_09_29/2011_09_26_drive_0051_sync/image_01/data/");

	CommandLineParser parser(argc,argv,keys);
    parser.about("Disparity Filtering Demo");
    if (parser.has("help"))
    {
        parser.printMessage();
        return 0;
    }
	//String left_im = parser.get<String>(0);
    //String right_im = parser.get<String>(1);

	std::vector<std::string> files_left = std::vector<std::string>();
	std::vector<std::string> files_right = std::vector<std::string>();

	getkittidir(dir_left, files_left);
	std::vector<cv::Mat> vector_src_left = getimages(dir_left, files_left);

	getkittidir(dir_right, files_right);
	std::vector<cv::Mat> vector_src_right = getimages(dir_right, files_right);

	if(vector_src_left.size() != vector_src_right.size()){
		return -1;
	}
	String GT_path = parser.get<String>("GT");
	String dst_path = parser.get<String>("dst_path");
	String dst_raw_path = parser.get<String>("dst_raw_path");
	String dst_conf_path = parser.get<String>("dst_conf_path");
	String algo = parser.get<String>("algorithm");
	String filter = parser.get<String>("filter");
	bool no_display = parser.has("no-display");
	bool no_downscale = parser.has("no-downscale");
	int max_disp = parser.get<int>("max_disparity");
	double lambda = parser.get<double>("wls_lambda");
	double sigma  = parser.get<double>("wls_sigma");
	double fbs_spatial = parser.get<double>("fbs_spatial");
	double fbs_luma = parser.get<double>("fbs_luma");
	double fbs_chroma = parser.get<double>("fbs_chroma");
	double fbs_lambda = parser.get<double>("fbs_lambda");
	double vis_mult = parser.get<double>("vis_mult");

	Mat left  = vector_src_left[0];
  	Mat right  = vector_src_right[0];

	int wsize;
	if(parser.get<int>("window_size")>=0) //user provided window_size value
		wsize = parser.get<int>("window_size");
	else
	{
		if(algo=="sgbm")
			wsize = 3; //default window size for SGBM
		else if(!no_downscale && algo=="bm" && filter=="wls_conf")
			wsize = 7; //default window size for BM on downscaled views (downscaling is performed only for wls_conf)
		else
			wsize = 15; //default window size for BM on full-sized views
	}

	if (!parser.check())
	{
		parser.printErrors();
		return -1;
	}

	//! [load_views]

	bool noGT = true;
	Mat GT_disp;
/*
	if (GT_path=="../data/aloeGT.png" && left_im!="../data/aloeL.jpg")
		noGT=true;
	else
	{
		noGT=false;
		if(readGT(GT_path,GT_disp)!=0)
		{
			cout<<"Cannot read ground truth image file: "<<GT_path<<endl;
			return -1;
		}
	}
*/
	Mat left_for_matcher, right_for_matcher;
	Mat left_disp,right_disp;
	Mat filtered_disp,solved_disp,solved_filtered_disp;
	Mat conf_map = Mat(left.rows,left.cols,CV_8U);
	conf_map = Scalar(255);
	Rect ROI;
	Ptr<DisparityWLSFilter> wls_filter;
	double matching_time, filtering_time;
	double solving_time = 0;
	if(max_disp<=0 || max_disp%16!=0)
	{
		cout<<"Incorrect max_disparity value: it should be positive and divisible by 16";
		return -1;
	}
	if(wsize<=0 || wsize%2!=1)
	{
		cout<<"Incorrect window_size value: it should be positive and odd";
		return -1;
	}
	for(int i = 0; i < vector_src_left.size(); i++){
		//! [load_views]
		left  = vector_src_left[i];
		if ( left.empty() )
		{
			//cout<<"Cannot read image file: "<<left_im;
			return -1;
		}

		right = vector_src_right[i];
		if ( right.empty() )
		{
			//cout<<"Cannot read image file: "<<right_im;
			return -1;
		}
		if(filter=="wls_conf") // filtering with confidence (significantly better quality than wls_no_conf)
		{
			if(!no_downscale)
			{
				// downscale the views to speed-up the matching stage, as we will need to compute both left
				// and right disparity maps for confidence map computation
				//! [downscale]
				max_disp/=2;
				if(max_disp%16!=0)
					max_disp += 16-(max_disp%16);
				resize(left ,left_for_matcher ,Size(),0.5,0.5, INTER_LINEAR_EXACT);
				resize(right,right_for_matcher,Size(),0.5,0.5, INTER_LINEAR_EXACT);
				//! [downscale]
			}
			else
			{
				left_for_matcher  = left.clone();
				right_for_matcher = right.clone();
			}

			if(algo=="bm")
			{
				//! [matching]
				Ptr<StereoBM> left_matcher = StereoBM::create(max_disp,wsize);
				wls_filter = createDisparityWLSFilter(left_matcher);
				Ptr<StereoMatcher> right_matcher = createRightMatcher(left_matcher);

				cvtColor(left_for_matcher,  left_for_matcher,  COLOR_BGR2GRAY);
				cvtColor(right_for_matcher, right_for_matcher, COLOR_BGR2GRAY);

				matching_time = (double)getTickCount();
				left_matcher-> compute(left_for_matcher, right_for_matcher,left_disp);
				right_matcher->compute(right_for_matcher,left_for_matcher, right_disp);
				matching_time = ((double)getTickCount() - matching_time)/getTickFrequency();
				//! [matching]
			}
			else if(algo=="sgbm")
			{
				Ptr<StereoSGBM> left_matcher  = StereoSGBM::create(0,max_disp,wsize);
				left_matcher->setP1(24*wsize*wsize);
				left_matcher->setP2(96*wsize*wsize);
				left_matcher->setPreFilterCap(63);
				left_matcher->setMode(StereoSGBM::MODE_SGBM_3WAY);
				wls_filter = createDisparityWLSFilter(left_matcher);
				Ptr<StereoMatcher> right_matcher = createRightMatcher(left_matcher);

				matching_time = (double)getTickCount();
				left_matcher-> compute(left_for_matcher, right_for_matcher,left_disp);
				right_matcher->compute(right_for_matcher,left_for_matcher, right_disp);
				matching_time = ((double)getTickCount() - matching_time)/getTickFrequency();
			}
			else
			{
				cout<<"Unsupported algorithm";
				return -1;
			}

			//! [filtering]
			wls_filter->setLambda(lambda);
			wls_filter->setSigmaColor(sigma);
			filtering_time = (double)getTickCount();
			wls_filter->filter(left_disp,left,filtered_disp,right_disp);
			filtering_time = ((double)getTickCount() - filtering_time)/getTickFrequency();
			//! [filtering]
			conf_map = wls_filter->getConfidenceMap();

			// Get the ROI that was used in the last filter call:
			ROI = wls_filter->getROI();
			if(!no_downscale)
			{
				// upscale raw disparity and ROI back for a proper comparison:
				resize(left_disp,left_disp,Size(),2.0,2.0,INTER_LINEAR_EXACT);
				left_disp = left_disp*2.0;
				ROI = Rect(ROI.x*2,ROI.y*2,ROI.width*2,ROI.height*2);
			}
		}
		else if(filter=="fbs_conf") // filtering with fbs and confidence using also wls pre-processing
		{
			if(!no_downscale)
			{
				// downscale the views to speed-up the matching stage, as we will need to compute both left
				// and right disparity maps for confidence map computation
				//! [downscale_wls]
				max_disp/=2;
				if(max_disp%16!=0)
					max_disp += 16-(max_disp%16);
				resize(left ,left_for_matcher ,Size(),0.5,0.5);
				resize(right,right_for_matcher,Size(),0.5,0.5);
				//! [downscale_wls]
			}
			else
			{
				left_for_matcher  = left.clone();
				right_for_matcher = right.clone();
			}

			if(algo=="bm")
			{
				//! [matching_wls]
				Ptr<StereoBM> left_matcher = StereoBM::create(max_disp,wsize);
				wls_filter = createDisparityWLSFilter(left_matcher);
				Ptr<StereoMatcher> right_matcher = createRightMatcher(left_matcher);

				cvtColor(left_for_matcher,  left_for_matcher,  COLOR_BGR2GRAY);
				cvtColor(right_for_matcher, right_for_matcher, COLOR_BGR2GRAY);

				matching_time = (double)getTickCount();
				left_matcher-> compute(left_for_matcher, right_for_matcher,left_disp);
				right_matcher->compute(right_for_matcher,left_for_matcher, right_disp);
				matching_time = ((double)getTickCount() - matching_time)/getTickFrequency();
				//! [matching_wls]
			}
			else if(algo=="sgbm")
			{
				Ptr<StereoSGBM> left_matcher  = StereoSGBM::create(0,max_disp,wsize);
				left_matcher->setP1(24*wsize*wsize);
				left_matcher->setP2(96*wsize*wsize);
				left_matcher->setPreFilterCap(63);
				left_matcher->setMode(StereoSGBM::MODE_SGBM_3WAY);
				wls_filter = createDisparityWLSFilter(left_matcher);
				Ptr<StereoMatcher> right_matcher = createRightMatcher(left_matcher);

				matching_time = (double)getTickCount();
				left_matcher-> compute(left_for_matcher, right_for_matcher,left_disp);
				right_matcher->compute(right_for_matcher,left_for_matcher, right_disp);
				matching_time = ((double)getTickCount() - matching_time)/getTickFrequency();
			}
			else
			{
				cout<<"Unsupported algorithm";
				return -1;
			}

			//! [filtering_wls]
			wls_filter->setLambda(lambda);
			wls_filter->setSigmaColor(sigma);
			filtering_time = (double)getTickCount();
			wls_filter->filter(left_disp,left,filtered_disp,right_disp);
			filtering_time = ((double)getTickCount() - filtering_time)/getTickFrequency();
			//! [filtering_wls]

			conf_map = wls_filter->getConfidenceMap();

			Mat left_disp_resized;
			resize(left_disp,left_disp_resized,left.size());

			// Get the ROI that was used in the last filter call:
			ROI = wls_filter->getROI();
			if(!no_downscale)
			{
				// upscale raw disparity and ROI back for a proper comparison:
				resize(left_disp,left_disp,Size(),2.0,2.0);
				left_disp = left_disp*2.0;
				left_disp_resized = left_disp_resized*2.0;
				ROI = Rect(ROI.x*2,ROI.y*2,ROI.width*2,ROI.height*2);
			}

	#ifdef HAVE_EIGEN
			//! [filtering_fbs]
			solving_time = (double)getTickCount();
			fastBilateralSolverFilter(left, left_disp_resized, conf_map/255.0f, solved_disp, fbs_spatial, fbs_luma, fbs_chroma, fbs_lambda);
			solving_time = ((double)getTickCount() - solving_time)/getTickFrequency();
			//! [filtering_fbs]

			//! [filtering_wls2fbs]
			fastBilateralSolverFilter(left, filtered_disp, conf_map/255.0f, solved_filtered_disp, fbs_spatial, fbs_luma, fbs_chroma, fbs_lambda);
			//! [filtering_wls2fbs]
	#else
			(void)fbs_spatial;
			(void)fbs_luma;
			(void)fbs_chroma;
			(void)fbs_lambda;
	#endif
		}
		else if(filter=="wls_no_conf")
		{
			/* There is no convenience function for the case of filtering with no confidence, so we
			will need to set the ROI and matcher parameters manually */

			left_for_matcher  = left.clone();
			right_for_matcher = right.clone();

			if(algo=="bm")
			{
				Ptr<StereoBM> matcher  = StereoBM::create(max_disp,wsize);
				matcher->setTextureThreshold(0);
				matcher->setUniquenessRatio(0);
				cvtColor(left_for_matcher,  left_for_matcher, COLOR_BGR2GRAY);
				cvtColor(right_for_matcher, right_for_matcher, COLOR_BGR2GRAY);
				ROI = computeROI(left_for_matcher.size(),matcher);
				wls_filter = createDisparityWLSFilterGeneric(false);
				wls_filter->setDepthDiscontinuityRadius((int)ceil(0.33*wsize));

				matching_time = (double)getTickCount();
				matcher->compute(left_for_matcher,right_for_matcher,left_disp);
				matching_time = ((double)getTickCount() - matching_time)/getTickFrequency();
			}
			else if(algo=="sgbm")
			{
				Ptr<StereoSGBM> matcher  = StereoSGBM::create(0,max_disp,wsize);
				matcher->setUniquenessRatio(0);
				matcher->setDisp12MaxDiff(1000000);
				matcher->setSpeckleWindowSize(0);
				matcher->setP1(24*wsize*wsize);
				matcher->setP2(96*wsize*wsize);
				matcher->setMode(StereoSGBM::MODE_SGBM_3WAY);
				ROI = computeROI(left_for_matcher.size(),matcher);
				wls_filter = createDisparityWLSFilterGeneric(false);
				wls_filter->setDepthDiscontinuityRadius((int)ceil(0.5*wsize));

				matching_time = (double)getTickCount();
				matcher->compute(left_for_matcher,right_for_matcher,left_disp);
				matching_time = ((double)getTickCount() - matching_time)/getTickFrequency();
			}
			else
			{
				cout<<"Unsupported algorithm";
				return -1;
			}

			wls_filter->setLambda(lambda);
			wls_filter->setSigmaColor(sigma);
			filtering_time = (double)getTickCount();
			wls_filter->filter(left_disp,left,filtered_disp,Mat(),ROI);
			filtering_time = ((double)getTickCount() - filtering_time)/getTickFrequency();
		}
		else
		{
			cout<<"Unsupported filter";
			return -1;
		}

		//collect and print all the stats:
	/*
		cout.precision(2);
		cout<<"Matching time:  "<<matching_time<<"s"<<endl;
		cout<<"Filtering time: "<<filtering_time<<"s"<<endl;
		cout<<"Solving time: "<<solving_time<<"s"<<endl;
		cout<<endl;
	*/
		double MSE_before,percent_bad_before,MSE_after,percent_bad_after;
		if(!noGT)
		{
			MSE_before = computeMSE(GT_disp,left_disp,ROI);
			percent_bad_before = computeBadPixelPercent(GT_disp,left_disp,ROI);
			MSE_after = computeMSE(GT_disp,filtered_disp,ROI);
			percent_bad_after = computeBadPixelPercent(GT_disp,filtered_disp,ROI);
	/*
			cout.precision(5);
			cout<<"MSE before filtering: "<<MSE_before<<endl;
			cout<<"MSE after filtering:  "<<MSE_after<<endl;
			cout<<endl;
			cout.precision(3);
			cout<<"Percent of bad pixels before filtering: "<<percent_bad_before<<endl;
			cout<<"Percent of bad pixels after filtering:  "<<percent_bad_after<<endl;
	*/
		}

		if(dst_path!="None")
		{
			Mat filtered_disp_vis;
			getDisparityVis(filtered_disp,filtered_disp_vis,vis_mult);
			imwrite(dst_path,filtered_disp_vis);
		}
		if(dst_raw_path!="None")
		{
			Mat raw_disp_vis;
			getDisparityVis(left_disp,raw_disp_vis,vis_mult);
			imwrite(dst_raw_path,raw_disp_vis);
		}
		if(dst_conf_path!="None")
		{
			imwrite(dst_conf_path,conf_map);
		}

		if(!no_display)
		{
			namedWindow("left", WINDOW_AUTOSIZE);
			imshow("left", left);
			namedWindow("right", WINDOW_AUTOSIZE);
			imshow("right", right);

			if(!noGT)
			{
				Mat GT_disp_vis;
				getDisparityVis(GT_disp,GT_disp_vis,vis_mult);
				namedWindow("ground-truth disparity", WINDOW_AUTOSIZE);
				imshow("ground-truth disparity", GT_disp_vis);
			}

			//! [visualization]
			Mat raw_disp_vis;
			/*
			getDisparityVis(left_disp,raw_disp_vis,vis_mult);
			namedWindow("raw disparity", WINDOW_AUTOSIZE);
			imshow("raw disparity", raw_disp_vis);
			*/
			Mat filtered_disp_vis;
			        // Holds the colormap version of the image:
			Mat cm_img0;
			// Apply the colormap:

			getDisparityVis(filtered_disp,filtered_disp_vis,vis_mult);
			applyColorMap(filtered_disp_vis*5, filtered_disp_vis, COLORMAP_JET);
			namedWindow("filtered disparity", WINDOW_AUTOSIZE);
			imshow("filtered disparity", filtered_disp_vis);

			if(!solved_disp.empty())
			{
				Mat solved_disp_vis;
				getDisparityVis(solved_disp,solved_disp_vis,vis_mult);
				namedWindow("solved disparity", WINDOW_AUTOSIZE);
				imshow("solved disparity", solved_disp_vis);

				Mat solved_filtered_disp_vis;
				getDisparityVis(solved_filtered_disp,solved_filtered_disp_vis,vis_mult);
				namedWindow("solved wls disparity", WINDOW_AUTOSIZE);
				imshow("solved wls disparity", solved_filtered_disp_vis);
			}
			cv::waitKey(10);
			//! [visualization]
		}
	}
    return 0;
}

Rect computeROI(Size2i src_sz, Ptr<StereoMatcher> matcher_instance)
{
    int min_disparity = matcher_instance->getMinDisparity();
    int num_disparities = matcher_instance->getNumDisparities();
    int block_size = matcher_instance->getBlockSize();

    int bs2 = block_size/2;
    int minD = min_disparity, maxD = min_disparity + num_disparities - 1;

    int xmin = maxD + bs2;
    int xmax = src_sz.width + minD - bs2;
    int ymin = bs2;
    int ymax = src_sz.height - bs2;

    Rect r(xmin, ymin, xmax - xmin, ymax - ymin);
    return r;
}

void stereo_playground(){
	std::shared_ptr<DepthEstimatorStrategy> depthestimate(new StereoBMConcrete());
	depthestimate->create();
	std::string dir_left = std::string("/media/prismadynamics/Elements/KITTI/raw/2011_09_29/2011_09_26_drive_0117_sync/image_00/data/");
	std::vector<std::string> files_left = std::vector<std::string>();
	getkittidir(dir_left, files_left);
	std::vector<cv::Mat> vector_src_left = getimages(dir_left, files_left);

	std::string dir_right = std::string("/media/prismadynamics/Elements/KITTI/raw/2011_09_29/2011_09_26_drive_0117_sync/image_01/data/");
	std::vector<std::string> files_right = std::vector<std::string>();
	getkittidir(dir_right, files_right);
	std::vector<cv::Mat> vector_src_right = getimages(dir_right, files_right);
	cv::Mat dst;
	cv::namedWindow("Disparity", WINDOW_AUTOSIZE);
	cv::namedWindow("Original", WINDOW_AUTOSIZE);
	for(int i = 0; i < files_left.size(); i++){
		depthestimate->get_disparity(vector_src_left[i],vector_src_right[i], dst);
		depthestimate->get_disparity_viz(dst,dst);
		cv::imshow("Disparity",dst);
		cv::imshow("Original",vector_src_left[i]);
		cv::waitKey(50);
	}
}

#include "DepthEstimatorStrategy.h"
#include "StereoBMConcrete.h"
void LOG(std::string text){
	std::cout << text << std::endl;
}
int main(int argc, char** argv) {
	SLAM::ImageUtility* ImgUtil = new SLAM::ImageUtility();
 	//playground_stereo_matching(argc, argv);
	stereo_playground();

	return 0;
}
