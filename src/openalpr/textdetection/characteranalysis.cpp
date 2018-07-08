/*
 * Copyright (c) 2015 OpenALPR Technology, Inc.
 * Open source Automated License Plate Recognition [http://www.openalpr.com]
 *
 * This file is part of OpenALPR.
 *
 * OpenALPR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License
 * version 3 as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <opencv2/imgproc/imgproc.hpp>

#include "characteranalysis.h"
#include "linefinder.h"

using namespace cv;
using namespace std;

namespace alpr
{

  bool sort_text_line(TextLine i, TextLine j) { return (i.topLine.p1.y < j.topLine.p1.y); }

  CharacterAnalysis::CharacterAnalysis(PipelineData* pipeline_data)
  {
    this->pipeline_data = pipeline_data;
    this->config = pipeline_data->config;

    if (this->config->debugCharAnalysis)
      cout << "Starting CharacterAnalysis identification" << endl;

    this->analyze();
  }

  CharacterAnalysis::~CharacterAnalysis()
  {

  }
  //q modify
  //去除二值图像边缘的突出部
  //uthreshold、vthreshold分别表示突出部的宽度阈值和高度阈值
  //type代表突出部的颜色，0表示黑色，1代表白色 
  void delete_jut(Mat& src, Mat& dst, int uthreshold, int vthreshold, int type)
  {
	  int threshold;
	  src.copyTo(dst);
	  int height = dst.rows;
	  int width = dst.cols;
	  int k;  //用于循环计数传递到外部
	  for (int i = 0; i < height - 1; i++)
	  {
		  uchar* p = dst.ptr<uchar>(i);
		  for (int j = 0; j < width - 1; j++)
		  {
			  if (type == 0)
			  {
				  //行消除
				  if (p[j] == 255 && p[j + 1] == 0)
				  {
					  if (j + uthreshold >= width)
					  {
						  for (int k = j + 1; k < width; k++)
							  p[k] = 255;
					  }
					  else
					  {
						  for (k = j + 2; k <= j + uthreshold; k++)
						  {
							  if (p[k] == 255) break;
						  }
						  if (p[k] == 255)
						  {
							  for (int h = j + 1; h < k; h++)
								  p[h] = 255;
						  }
					  }
				  }
				  //列消除
				  if (p[j] == 255 && p[j + width] == 0)
				  {
					  if (i + vthreshold >= height)
					  {
						  for (k = j + width; k < j + (height - i)*width; k += width)
							  p[k] = 255;
					  }
					  else
					  {
						  for (k = j + 2 * width; k <= j + vthreshold*width; k += width)
						  {
							  if (p[k] == 255) break;
						  }
						  if (p[k] == 255)
						  {
							  for (int h = j + width; h < k; h += width)
								  p[h] = 255;
						  }
					  }
				  }
			  }
			  else  //type = 1
			  {
				  //行消除
				  if (p[j] == 0 && p[j + 1] == 255)
				  {
					  if (j + uthreshold >= width)
					  {
						  for (int k = j + 1; k < width; k++)
							  p[k] = 0;
					  }
					  else
					  {
						  for (k = j + 2; k <= j + uthreshold; k++)
						  {
							  if (p[k] == 0) break;
						  }
						  if (p[k] == 0)
						  {
							  for (int h = j + 1; h < k; h++)
								  p[h] = 0;
						  }
					  }
				  }
				  //列消除
				  if (p[j] == 0 && p[j + width] == 255)
				  {
					  if (i + vthreshold >= height)
					  {
						  for (k = j + width; k < j + (height - i)*width; k += width)
							  p[k] = 0;
					  }
					  else
					  {
						  for (k = j + 2 * width; k <= j + vthreshold*width; k += width)
						  {
							  if (p[k] == 0) break;
						  }
						  if (p[k] == 0)
						  {
							  for (int h = j + width; h < k; h += width)
								  p[h] = 0;
						  }
					  }
				  }
			  }
		  }
	  }
  }
  //图片边缘光滑处理
  //size表示取均值的窗口大小，threshold表示对均值图像进行二值化的阈值
  void imageblur(Mat& src, Mat& dst, Size size, int threshold)
  {
	  int height = src.rows;
	  int width = src.cols;
	  blur(src, dst, size);
	  for (int i = 0; i < height; i++)
	  {
		  uchar* p = dst.ptr<uchar>(i);
		  for (int j = 0; j < width; j++)
		  {
			  if (p[j] < threshold)
				  p[j] = 0;
			  else p[j] = 255;
		  }
	  }
	  imshow("Blur", dst);
  }


  //去除车牌上方的钮钉
  //计算每行元素的阶跃数，如果小于X认为是柳丁，将此行全部填0（涂黑）
  // X的推荐值为，可根据实际调整
  bool clearLiuDing(Mat& img) {
	  vector<float> fJump;
	  int whiteCount = 0;
	  const int x = 7;
	  Mat jump = Mat::zeros(1, img.rows, CV_32F);
	  for (int i = 0; i < img.rows; i++) {
		  int jumpCount = 0;

		  for (int j = 0; j < img.cols - 1; j++) {
			  if (img.at<char>(i, j) != img.at<char>(i, j + 1)) jumpCount++;

			  if (img.at<uchar>(i, j) == 255) {
				  whiteCount++;
			  }
		  }

		  jump.at<float>(i) = (float)jumpCount;
	  }

	  int iCount = 0;
	  for (int i = 0; i < img.rows; i++) {
		  fJump.push_back(jump.at<float>(i));
		  if (jump.at<float>(i) >= 16 && jump.at<float>(i) <= 45) {
			  //车牌字符满足一定跳变条件
			  iCount++;
		  }
	  }

	  ////这样的不是车牌
	  if (iCount * 1.0 / img.rows <= 0.40) {
		  //满足条件的跳变的行数也要在一定的阈值内
		  return false;
	  }
	  //不满足车牌的条件
	  if (whiteCount * 1.0 / (img.rows * img.cols) < 0.15 ||
		  whiteCount * 1.0 / (img.rows * img.cols) > 0.50) {
		  return false;
	  }

	  for (int i = 0; i < img.rows; i++) {
		  if (jump.at<float>(i) <= x) {
			  for (int j = 0; j < img.cols; j++) {
				  img.at<char>(i, j) = 0;
			  }
		  }
	  }
	  return true;
  }

  //q modify
  void CharacterAnalysis::analyze()
  {
    timespec startTime;
    getTimeMonotonic(&startTime);

    if (config->always_invert)
      bitwise_not(pipeline_data->crop_gray, pipeline_data->crop_gray);

    pipeline_data->clearThresholds();
	//生成3个二值化图像
    pipeline_data->thresholds = produceThresholds(pipeline_data->crop_gray, config);

	//q modify
	//为了得到字符是否需要invert，把之前的处理的结果作为一个预处理
	{
		timespec contoursStartTime;
		getTimeMonotonic(&contoursStartTime);

		pipeline_data->textLines.clear();

		for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
		{
		  TextContours tc(pipeline_data->thresholds[i]);

		  allTextContours.push_back(tc);
		}

		//Mat img_equalized = equalizeBrightness(img_gray);

		timespec filterStartTime;
		getTimeMonotonic(&filterStartTime);

		for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
		{
		  //滤波 
		  //输入：二值化影像和所有轮廓
		  //输出：好的字符轮廓
		  this->filter(pipeline_data->thresholds[i], allTextContours[i]);

		  if (config->debugCharAnalysis)
			cout << "Threshold " << i << " had " << allTextContours[i].getGoodIndicesCount() << " good indices." << endl;
		}

		//获得好的字符轮廓的掩模
		PlateMask plateMask(pipeline_data);

		plateMask.findOuterBoxMask(allTextContours);

		pipeline_data->hasPlateBorder = plateMask.hasPlateMask;
		pipeline_data->plateBorderMask = plateMask.getMask();


		if (plateMask.hasPlateMask)
		{
		  // Filter out bad contours now that we have an outer box mask...
		  for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
		  {
			//根据字符轮廓掩模，滤波获得好的优秀的字符
			filterByOuterMask(allTextContours[i]);

		  }
		}


		int bestFitScore = -1;
		int bestFitIndex = -1;
		for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
		{

		  int segmentCount = allTextContours[i].getGoodIndicesCount();

		  if (segmentCount > bestFitScore)
		  {
			bestFitScore = segmentCount;
			bestFitIndex = i;
			//最好的二值化图像
			bestThreshold = pipeline_data->thresholds[i];
			//最好的字符轮廓
			bestContours = allTextContours[i];
		  }
		}
	}

	//q modify


	//q modify
	//注释掉此部分
	//pipeline_data->plate_inverted = true，代表需要转换，字符需要变白，后面bsetbox是基于白色字体
	//因为前面已经判断过一次是否字符需要变白了，且改变了二值化图，所以此处不需要再判断了，判断一次就够了，
	//就够对裁剪后的新图是否进行二值化反操作了（bitwise_not）
	//即保证了找边框和后面的找字符都使用白色字
	//识别字体和底色的二值图，保证字体是二值图的白色

    if (config->auto_invert)
      pipeline_data->plate_inverted = isPlateInverted();
    else
      pipeline_data->plate_inverted = config->always_invert;

	//q modify
    if (config->debugGeneral)
      cout << "Plate inverted: " << pipeline_data->plate_inverted << endl;
    
	//q modify
	allTextContours.clear();
	//对字符轮廓做一次平滑
	//this->filter(pipeline_data->thresholds[0], TextContours(pipeline_data->thresholds[0]));
	//bestThreshold = pipeline_data->thresholds[0];
	//if (config->auto_invert)
	//	pipeline_data->plate_inverted = isPlateInverted();
	//else
	//	pipeline_data->plate_inverted = config->always_invert;
	if (pipeline_data->plate_inverted)
	{
		for (int i = 0; i < pipeline_data->thresholds.size(); i++) {
			bitwise_not(pipeline_data->thresholds[i], pipeline_data->thresholds[i]);
		}
	}
	for (int i = 0; i < pipeline_data->thresholds.size(); i++)
	{
		Mat dst;
		delete_jut(pipeline_data->thresholds[i], dst, 1, 1, pipeline_data->plate_inverted);
		dst.copyTo(pipeline_data->thresholds[i]);
		clearLiuDing(pipeline_data->thresholds[i]);
		//blur(dst, dst, dst.size());
	}


	timespec contoursStartTime;
	getTimeMonotonic(&contoursStartTime);

	pipeline_data->textLines.clear();

	for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
	{
		TextContours tc(pipeline_data->thresholds[i]);

		allTextContours.push_back(tc);
	}

	//q debug
	Mat one = allTextContours[0].drawDebugImage(pipeline_data->thresholds[0]);
	Mat two = allTextContours[1].drawDebugImage(pipeline_data->thresholds[1]);
	Mat three = allTextContours[2].drawDebugImage(pipeline_data->thresholds[2]);
	//q debug

	if (config->debugTiming)
	{
		timespec contoursEndTime;
		getTimeMonotonic(&contoursEndTime);
		cout << "  -- Character Analysis Find Contours Time: " << diffclock(contoursStartTime, contoursEndTime) << "ms." << endl;
	}
	//Mat img_equalized = equalizeBrightness(img_gray);

	timespec filterStartTime;
	getTimeMonotonic(&filterStartTime);

	for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
	{
		//滤波 
		//输入：二值化影像和所有轮廓
		//输出：好的字符轮廓
		this->filter(pipeline_data->thresholds[i], allTextContours[i]);

		if (config->debugCharAnalysis)
			cout << "Threshold " << i << " had " << allTextContours[i].getGoodIndicesCount() << " good indices." << endl;
	}

	//q debug
	Mat one1 = allTextContours[0].drawDebugImage(pipeline_data->thresholds[0]);
	Mat two1 = allTextContours[1].drawDebugImage(pipeline_data->thresholds[1]);
	Mat three1 = allTextContours[2].drawDebugImage(pipeline_data->thresholds[2]);
	//q debug

	if (config->debugTiming)
	{
		timespec filterEndTime;
		getTimeMonotonic(&filterEndTime);
		cout << "  -- Character Analysis Filter Time: " << diffclock(filterStartTime, filterEndTime) << "ms." << endl;
	}

	//获得好的字符轮廓的掩模
	PlateMask plateMask(pipeline_data);

	plateMask.findOuterBoxMask(allTextContours);

	pipeline_data->hasPlateBorder = plateMask.hasPlateMask;
	pipeline_data->plateBorderMask = plateMask.getMask();


	if (plateMask.hasPlateMask)
	{
		// Filter out bad contours now that we have an outer box mask...
		for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
		{
			//根据字符轮廓掩模，滤波获得好的优秀的字符

			filterByOuterMask(allTextContours[i]);

		}
	}
	//q debug
	Mat one2 = allTextContours[0].drawDebugImage(pipeline_data->thresholds[0]);
	Mat two2 = allTextContours[1].drawDebugImage(pipeline_data->thresholds[1]);
	Mat three2 = allTextContours[2].drawDebugImage(pipeline_data->thresholds[2]);
	//q debug


	int bestFitScore = -1;
	int bestFitIndex = -1;
	for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
	{

		int segmentCount = allTextContours[i].getGoodIndicesCount();

		if (segmentCount > bestFitScore)
		{
			bestFitScore = segmentCount;
			bestFitIndex = i;
			//最好的二值化图像
			bestThreshold = pipeline_data->thresholds[i];
			//最好的字符轮廓
			bestContours = allTextContours[i];
		}
	}

	if (this->config->debugCharAnalysis)
		cout << "Best fit score: " << bestFitScore << " Index: " << bestFitIndex << endl;

	if (bestFitScore <= 1)
	{
		pipeline_data->disqualified = true;
		pipeline_data->disqualify_reason = "Low best fit score in characteranalysis";
		return;
	}

	//getColorMask(img, allContours, allHierarchy, charSegments);

	//显示滤波后的所有的字符轮廓，绿色为好的，蓝色为差的
	if (this->config->debugCharAnalysis)
	{
		Mat img_contours = bestContours.drawDebugImage(bestThreshold);

		displayImage(config, "Matching Contours", img_contours);
	}





	//q modify

    // Invert multiline plates and redo the thresholds before finding the second line
    if (config->multiline && config->auto_invert && pipeline_data->plate_inverted)
    {
      bitwise_not(pipeline_data->crop_gray, pipeline_data->crop_gray);
      pipeline_data->thresholds = produceThresholds(pipeline_data->crop_gray, pipeline_data->config);
    }
      
    
    LineFinder lf(pipeline_data);
	//得到扩展后的线，如果是双行，有两条线
    vector<vector<Point> > linePolygons = lf.findLines(pipeline_data->crop_gray, bestContours);
	//TextLine是字符线，即包含了 扩展线，优选字符线，优选字符矩形，字符平均高度，和线的角度的类
    vector<TextLine> tempTextLines;
    for (unsigned int i = 0; i < linePolygons.size(); i++)
    {
      vector<Point> linePolygon = linePolygons[i];

      LineSegment topLine = LineSegment(linePolygon[0].x, linePolygon[0].y, linePolygon[1].x, linePolygon[1].y);
      LineSegment bottomLine = LineSegment(linePolygon[3].x, linePolygon[3].y, linePolygon[2].x, linePolygon[2].y);
	  //得到优选字符的范围，框框
      vector<Point> textArea = getCharArea(topLine, bottomLine);

      TextLine textLine(textArea, linePolygon, pipeline_data->crop_gray.size());

      tempTextLines.push_back(textLine);
    }
	//根据优选矩形再对bestThreshold进行滤波
    filterBetweenLines(bestThreshold, bestContours, tempTextLines);

    // Sort the lines from top to bottom.
    std::sort(tempTextLines.begin(), tempTextLines.end(), sort_text_line);

    // Now that we've filtered a few more contours, re-do the text area.
	//得到再次滤波后的textLines
    for (unsigned int i = 0; i < tempTextLines.size(); i++)
    {
      vector<Point> updatedTextArea = getCharArea(tempTextLines[i].topLine, tempTextLines[i].bottomLine);
      vector<Point> linePolygon = tempTextLines[i].linePolygon;
      if (updatedTextArea.size() > 0 && linePolygon.size() > 0)
      {
        pipeline_data->textLines.push_back(TextLine(updatedTextArea, linePolygon, pipeline_data->crop_gray.size()));
      }

    }


    if (pipeline_data->textLines.size() > 0)
    {
      int confidenceDrainers = 0;
      int charSegmentCount = this->bestContours.getGoodIndicesCount();
      if (charSegmentCount == 1)
        confidenceDrainers += 91;
      else if (charSegmentCount < 5)
        confidenceDrainers += (5 - charSegmentCount) * 10;

      // Use the angle for the first line -- assume they'll always be parallel for multi-line plates
      int absangle = abs(pipeline_data->textLines[0].topLine.angle);
      if (absangle > config->maxPlateAngleDegrees)
        confidenceDrainers += 91;
      else if (absangle > 1)
        confidenceDrainers += absangle ;

      // If a multiline plate has only one line, disqualify
      if (pipeline_data->isMultiline && pipeline_data->textLines.size() < 2)
      {
        if (config->debugCharAnalysis)
          std::cout << "Did not detect multiple lines on multi-line plate" << std::endl;
        confidenceDrainers += 95;
      }

      if (confidenceDrainers >= 90)
      {
        pipeline_data->disqualified = true;
        pipeline_data->disqualify_reason = "Low confidence in characteranalysis";
      }
      else
      {
        float confidence = 100 - confidenceDrainers;
        pipeline_data->confidence_weights.setScore("CHARACTER_ANALYSIS_SCORE", confidence, 1.0);
      }
    }
    else
    {
        pipeline_data->disqualified = true;
        pipeline_data->disqualify_reason = "No text lines found in characteranalysis";
    }

    if (config->debugTiming)
    {
      timespec endTime;
      getTimeMonotonic(&endTime);
      cout << "Character Analysis Time: " << diffclock(startTime, endTime) << "ms." << endl;
    }

	//q debug
	if (pipeline_data->config->q_debug && !this->pipeline_data->config->debugCharAnalysis && pipeline_data->textLines.size() > 0)
	{
		vector<Mat> tempDash;
		for (unsigned int z = 0; z < pipeline_data->thresholds.size(); z++)
		{
			Mat tmp(pipeline_data->thresholds[z].size(), pipeline_data->thresholds[z].type());
			pipeline_data->thresholds[z].copyTo(tmp);
			cvtColor(tmp, tmp, CV_GRAY2BGR);

			tempDash.push_back(tmp);
		}

		Mat bestVal(this->bestThreshold.size(), this->bestThreshold.type());
		this->bestThreshold.copyTo(bestVal);
		cvtColor(bestVal, bestVal, CV_GRAY2BGR);

		for (unsigned int z = 0; z < this->bestContours.size(); z++)
		{
			Scalar dcolor(255, 0, 0);
			if (this->bestContours.goodIndices[z])
				dcolor = Scalar(0, 255, 0);
			drawContours(bestVal, this->bestContours.contours, z, dcolor, 1);
		}
		tempDash.push_back(bestVal);

		std::string str = pipeline_data->config->fileName;
		ostringstream oss;
		oss << pipeline_data->config->q_debug;
		std::string name = pipeline_data->config->outputPath + str + "-" + oss.str() + ".jpg";
		imwrite(name, drawImageDashboard(tempDash, bestVal.type(), 3));
		pipeline_data->config->q_debug++;
	}
	//q debug

    // Draw debug dashboard
    if (this->pipeline_data->config->debugCharAnalysis && pipeline_data->textLines.size() > 0)
    {
      vector<Mat> tempDash;
      for (unsigned int z = 0; z < pipeline_data->thresholds.size(); z++)
      {
        Mat tmp(pipeline_data->thresholds[z].size(), pipeline_data->thresholds[z].type());
        pipeline_data->thresholds[z].copyTo(tmp);
        cvtColor(tmp, tmp, CV_GRAY2BGR);

        tempDash.push_back(tmp);
      }

      Mat bestVal(this->bestThreshold.size(), this->bestThreshold.type());
      this->bestThreshold.copyTo(bestVal);
      cvtColor(bestVal, bestVal, CV_GRAY2BGR);

      for (unsigned int z = 0; z < this->bestContours.size(); z++)
      {
        Scalar dcolor(255,0,0);
        if (this->bestContours.goodIndices[z])
          dcolor = Scalar(0,255,0);
        drawContours(bestVal, this->bestContours.contours, z, dcolor, 1);
      }
      tempDash.push_back(bestVal);
      displayImage(config, "Character Region Step 1 Thresholds", drawImageDashboard(tempDash, bestVal.type(), 3));
    }
  }




  Mat CharacterAnalysis::getCharacterMask()
  {
    Mat charMask = Mat::zeros(bestThreshold.size(), CV_8U);

    for (unsigned int i = 0; i < bestContours.size(); i++)
    {
      if (bestContours.goodIndices[i] == false)
        continue;

      drawContours(charMask, bestContours.contours,
                   i, // draw this contour
                   cv::Scalar(255,255,255), // in
                   CV_FILLED,
                   8,
                   bestContours.hierarchy,
                   1
                  );

    }

    return charMask;
  }


  void CharacterAnalysis::filter(Mat img, TextContours& textContours)
  {
    int STARTING_MIN_HEIGHT = round (((float) img.rows) * config->charAnalysisMinPercent);
    int STARTING_MAX_HEIGHT = round (((float) img.rows) * (config->charAnalysisMinPercent + config->charAnalysisHeightRange));
    int HEIGHT_STEP = round (((float) img.rows) * config->charAnalysisHeightStepSize);
    int NUM_STEPS = config->charAnalysisNumSteps;

    int bestFitScore = -1;

    vector<bool> bestIndices;
     
    for (int i = 0; i < NUM_STEPS; i++)
    {

      //vector<bool> goodIndices(contours.size());
      for (unsigned int z = 0; z < textContours.size(); z++) textContours.goodIndices[z] = true;

      this->filterByBoxSize(textContours, STARTING_MIN_HEIGHT + (i * HEIGHT_STEP), STARTING_MAX_HEIGHT + (i * HEIGHT_STEP));

      int goodIndices = textContours.getGoodIndicesCount();
      if ( goodIndices == 0 || goodIndices <= bestFitScore)	// Don't bother doing more filtering if we already lost...
        continue;

      this->filterContourHoles(textContours);

      goodIndices = textContours.getGoodIndicesCount();
      if ( goodIndices == 0 || goodIndices <= bestFitScore)	// Don't bother doing more filtering if we already lost...
        continue;


      int segmentCount = textContours.getGoodIndicesCount();

      if (segmentCount > bestFitScore)
      {
        bestFitScore = segmentCount;
        bestIndices = textContours.getIndicesCopy();
      }
    }

    textContours.setIndices(bestIndices);
  }

  // Goes through the contours for the plate and picks out possible char segments based on min/max height
  void CharacterAnalysis::filterByBoxSize(TextContours& textContours, int minHeightPx, int maxHeightPx)
  {
    // For multiline plates, we want to target the biggest line for character analysis, since it should be easier to spot.
    float larger_char_height_mm = 0;
    float larger_char_width_mm = 0;
    for (unsigned int i = 0; i < config->charHeightMM.size(); i++)
    {
      if (config->charHeightMM[i] > larger_char_height_mm)
      {
        larger_char_height_mm = config->charHeightMM[i];
        larger_char_width_mm = config->charWidthMM[i];
      }
    }
    
    float idealAspect=larger_char_width_mm / larger_char_height_mm;
    float aspecttolerance=0.25;


    for (unsigned int i = 0; i < textContours.size(); i++)
    {
      if (textContours.goodIndices[i] == false)
        continue;

      textContours.goodIndices[i] = false;  // Set it to not included unless it proves valid

      //Create bounding rect of object
      Rect mr= boundingRect(textContours.contours[i]);

      float minWidth = mr.height * 0.2;
      //Crop image

      //cout << "Height: " << minHeightPx << " - " << mr.height << " - " << maxHeightPx << " ////// Width: " << mr.width << " - " << minWidth << endl;
      if(mr.height >= minHeightPx && mr.height <= maxHeightPx && mr.width > minWidth)
      {
        float charAspect= (float)mr.width/(float)mr.height;

        //cout << "  -- stage 2 aspect: " << abs(charAspect) << " - " << aspecttolerance << endl;
        if (abs(charAspect - idealAspect) < aspecttolerance)
          textContours.goodIndices[i] = true;
      }
    }
	//q modify
	//优选框的字符应该在中心线附近，即其包围盒的y的中点坐标在高的0.3-0.7的范围内
	std::vector<Rect> goodbox;
	for (unsigned int i = 0; i < textContours.size(); i++)
	{
		if (textContours.goodIndices[i] == false)
			continue;

		Rect rect = boundingRect(textContours.contours[i]);
		goodbox.push_back(rect);
		float ymid = rect.y + 0.5*rect.width;
		if (config->templateHeightPx*0.3 < ymid && ymid < config->templateHeightPx*0.7)
			textContours.goodIndices[i] = true;
		else
			textContours.goodIndices[i] = false;
	}

	//外接矩形不能重合
	//clearGoodBox();
	//std::vector<Rect> okbox;
	//std::vector<Rect> badbox;
	//for (int i = 0; i < goodbox.size(); i++) {
	//	Rect rect = goodbox[i];
	//	bool good = true;
	//	for (int j = 0; j < goodbox.size(); j++) {
	//		if (j = i)
	//			continue;
	//		Rect rectAnother = goodbox[j];
	//		cv::Point p1(rectAnother.x, rectAnother.y);
	//		cv::Point p2(rectAnother.x + rectAnother.width, rectAnother.y + rectAnother.height);
	//		cv::Point p3(rectAnother.x, rectAnother.y + rectAnother.height);
	//		cv::Point p4(rectAnother.x + rectAnother.width, rectAnother.y);
	//		if (rect.contains(p1) && rect.contains(p2)&& rect.contains(p3) && rect.contains(p4)) {
	//			good = false;
	//		}
	//	}
	//	if (good) {
	//		okbox.push_back(rect);
	//	}
	//}


	//if(okbox.size())






	//算法2，滑动窗口法
	//1.求平均的优选字符的大小，并构造一个候选窗口A
	//2.从最左边的最优字符开始，让A沿，最优字符们的中线滑动
	//3.计算框到的轮廓的最小外界矩形B，如果B的大小类似于A，则认为是框到了汉字。设为优选字符。
	int a = textContours.getGoodIndicesCount();
	if (a < 3)
		return;
	float aver_width = 0;
	float aver_heigh = 0;
	float aver_y = 0;
	int minleftX = 99999;
	int maxleftX = 0;
	for (int i = 0; i < goodbox.size(); i++) {
		aver_heigh += goodbox[i].height;
		aver_width += goodbox[i].width;
		aver_y     += goodbox[i].y;
		minleftX    = min(minleftX, goodbox[i].x);
		maxleftX    = min(minleftX, goodbox[i].x);

	}

	aver_heigh /= goodbox.size();
	aver_width /= goodbox.size();
	aver_y     /= goodbox.size();

	float extern_w = aver_width*1.2;
	float extern_h = aver_heigh*1.2;

	vector<int> selectIdx;
	for (unsigned int i = 0; i < textContours.size(); i++)
	{
		if (textContours.goodIndices[i] == false)
		{
			Rect rect = boundingRect(textContours.contours[i]);
			if (rect.width < extern_w&&rect.height < extern_h &&rect.x + rect.width<minleftX)
			{
				//if(textContours.hierarchy[i][2] == -1)
				selectIdx.push_back(i);
			}
		}
	}

	int endX = minleftX - ceil(aver_width);
	if (endX < 0)
		return;
	//融合左边字符
	for (int i = 0; i < endX; i++) {
		//int moveWindow_x = i;
		int moveWindow_y = floor(aver_y + 0.5*aver_heigh-0.5*extern_h);
		Rect moveWindow(i, moveWindow_y, floor(extern_w), floor(extern_h));
		vector<cv::Point> Chinesecontours;
		for (int j = 0; j < selectIdx.size(); j++) {
			Rect rect = boundingRect(textContours.contours[selectIdx[j]]);
			cv::Point p1(rect.x, rect.y);
			cv::Point p2(rect.x + rect.width, rect.y + rect.height);
			if (moveWindow.contains(p1) && moveWindow.contains(p2))
			{
				Chinesecontours.insert(Chinesecontours.end(), textContours.contours[selectIdx[j]].begin(), textContours.contours[selectIdx[j]].end());
				//Rect rect = boundingRect(textContours.contours[selectIdx[j]]);
			}
		}
		Rect ChineseRect = boundingRect(Chinesecontours);
		if (ChineseRect.height >= minHeightPx && ChineseRect.height <= maxHeightPx && ChineseRect.width > ChineseRect.height * 0.2)
		{
			float charAspect = (float)ChineseRect.width / (float)ChineseRect.height;

			//cout << "  -- stage 2 aspect: " << abs(charAspect) << " - " << aspecttolerance << endl;
			if (abs(charAspect - idealAspect) < aspecttolerance) {
				textContours.contours.push_back(Chinesecontours);
				textContours.goodIndices.push_back(true);
				textContours.hierarchy.push_back(cv::Vec4i(-1, -1, -1, -1));
				return;
			}

		}

	}


	//融合右边字符
	//selectIdx.clear();
	//for (unsigned int i = 0; i < textContours.size(); i++)
	//{
	//	if (textContours.goodIndices[i] == false)
	//	{
	//		Rect rect = boundingRect(textContours.contours[i]);
	//		if (rect.width < extern_w&&rect.height < extern_h &&rect.x>maxleftX)
	//		{
	//			//if(textContours.hierarchy[i][2] == -1)
	//			selectIdx.push_back(i);
	//		}
	//	}
	//}

	//{
	//	int moveWindow_x = config->templateWidthPx - floor(extern_w);
	//	int moveWindow_y = floor(aver_y + 0.5*aver_heigh - 0.5*extern_h);
	//	Rect moveWindow(moveWindow_x, moveWindow_y, floor(extern_w), floor(extern_h));
	//	vector<cv::Point> Chinesecontours;
	//	for (int j = 0; j < selectIdx.size(); j++) {
	//		Rect rect = boundingRect(textContours.contours[selectIdx[j]]);
	//		cv::Point p1(rect.x, rect.y);
	//		cv::Point p2(rect.x + rect.width, rect.y + rect.height);
	//		if (moveWindow.contains(p1) && moveWindow.contains(p2))
	//		{
	//			Chinesecontours.insert(Chinesecontours.end(), textContours.contours[selectIdx[j]].begin(), textContours.contours[selectIdx[j]].end());
	//			//Rect rect = boundingRect(textContours.contours[selectIdx[j]]);
	//		}
	//	}
	//	Rect ChineseRect = boundingRect(Chinesecontours);
	//	if (ChineseRect.height >= minHeightPx && ChineseRect.height <= maxHeightPx && ChineseRect.width > ChineseRect.height * 0.2)
	//	{
	//		float charAspect = (float)ChineseRect.width / (float)ChineseRect.height;

	//		//cout << "  -- stage 2 aspect: " << abs(charAspect) << " - " << aspecttolerance << endl;
	//		if (abs(charAspect - idealAspect) < aspecttolerance) {
	//			textContours.contours.push_back(Chinesecontours);
	//			textContours.goodIndices.push_back(true);
	//			textContours.hierarchy.push_back(cv::Vec4i(-1, -1, -1, -1));
	//			return;
	//		}

	//	}
	//}


	//计算优选框的平均大小   找底座法，有点容易受杂质干扰
	//找个底座，然后画个平均方形，如果外接矩形在在方形内，就加入其中，然后再计算外界矩形，计算到一个就break了，效果不好
	//由正确的由30降到了24
	//int a = textContours.getGoodIndicesCount();
	//float aver_width = 0;
	//float aver_heigh = 0;
	//for (int i = 0; i < goodbox.size(); i++) {
	//	aver_heigh += goodbox[i].height;
	//	aver_width += goodbox[i].width;


	//}
	//aver_heigh /= goodbox.size();
	//aver_width /= goodbox.size();
	//float extern_w = aver_width*1.2;
	//float extern_h = aver_heigh*1.2;
	//vector<int> selectIdx;
	//for (unsigned int i = 0; i < textContours.size(); i++)
	//{
	//	if (textContours.goodIndices[i] == false)
	//	{
	//		Rect rect = boundingRect(textContours.contours[i]);
	//		if (rect.width < extern_w&&rect.height < extern_h &&rect.x+rect.width<0.25*config->templateWidthPx)
	//		{
	//			//if(textContours.hierarchy[i][2] == -1)
	//			selectIdx.push_back(i);
	//		}
	//	}
	//}
	//vector<cv::Point> Chinesecontours;
	//bool ismerge = false;
	//for (int i = 0; i < selectIdx.size(); i++) {
	//	Rect rect = boundingRect(textContours.contours[selectIdx[i]]);
	//	Rect newRect;
	//	newRect.width = extern_w;
	//	newRect.height = extern_h;
	//	newRect.x = rect.x - 0.5*(newRect.width - rect.width);
	//	newRect.y = rect.y - (newRect.height - rect.height);
	//	Chinesecontours = textContours.contours[selectIdx[i]];
	//	for (int j = 0; j < selectIdx.size(); j++)
	//	{
	//		if (j == i)
	//			continue;
	//		Rect rect = boundingRect(textContours.contours[selectIdx[j]]);
	//		cv::Point p1(rect.x, rect.y);
	//		cv::Point p2(rect.x+rect.width, rect.y+rect.height);
	//		if (newRect.contains(p1) && newRect.contains(p2))
	//		{
	//			Chinesecontours.insert(Chinesecontours.end(), textContours.contours[selectIdx[j]].begin(), textContours.contours[selectIdx[j]].end());
	//			ismerge = true;
	//		}
	//	}
	//	if (ismerge)
	//	{
	//		textContours.contours.push_back(Chinesecontours);
	//		textContours.goodIndices.push_back(true);
	//		textContours.hierarchy.push_back(cv::Vec4i(-1, -1, -1, -1));
	//		break;
	//	}
	//	

	//}

	//for (unsigned int i = 0; i < textContours.size(); i++)
	//{
	//	if (textContours.goodIndices[i] == false)
	//		continue;

	//	Rect rect = boundingRect(textContours.contours[i]);
	//	//goodbox.push_back(rect);
	//	float ymid = rect.y + 0.5*rect.width;
	//	if (config->templateHeightPx*0.3 < ymid && ymid < config->templateHeightPx*0.7)
	//		textContours.goodIndices[i] = true;
	//	else
	//		textContours.goodIndices[i] = false;
	//}

	//q modify


  }

  void CharacterAnalysis::filterContourHoles(TextContours& textContours)
  {

    for (unsigned int i = 0; i < textContours.size(); i++)
    {
      if (textContours.goodIndices[i] == false)
        continue;

      textContours.goodIndices[i] = false;  // Set it to not included unless it proves valid

      int parentIndex = textContours.hierarchy[i][3];

      if (parentIndex >= 0 && textContours.goodIndices[parentIndex])
      {
        // this contour is a child of an already identified contour.  REMOVE it
        if (this->config->debugCharAnalysis)
        {
          cout << "filterContourHoles: contour index: " << i << endl;
        }
      }
      else
      {
        textContours.goodIndices[i] = true;
      }
    }

  }

  // Goes through the contours for the plate and picks out possible char segments based on min/max height
  // returns a vector of indices corresponding to valid contours
  void CharacterAnalysis::filterByParentContour( TextContours& textContours)
  {

    vector<int> parentIDs;
    vector<int> votes;

    for (unsigned int i = 0; i < textContours.size(); i++)
    {
      if (textContours.goodIndices[i] == false)
        continue;

      textContours.goodIndices[i] = false;  // Set it to not included unless it proves 

      int voteIndex = -1;
      int parentID = textContours.hierarchy[i][3];
      // check if parentID is already in the lsit
      for (unsigned int j = 0; j < parentIDs.size(); j++)
      {
        if (parentIDs[j] == parentID)
        {
          voteIndex = j;
          break;
        }
      }
      if (voteIndex == -1)
      {
        parentIDs.push_back(parentID);
        votes.push_back(1);
      }
      else
      {
        votes[voteIndex] = votes[voteIndex] + 1;
      }
    }

    // Tally up the votes, pick the winner
    int totalVotes = 0;
    int winningParentId = 0;
    int highestVotes = 0;
    for (unsigned int i = 0; i < parentIDs.size(); i++)
    {
      if (votes[i] > highestVotes)
      {
        winningParentId = parentIDs[i];
        highestVotes = votes[i];
      }
      totalVotes += votes[i];
    }

    // Now filter out all the contours with a different parent ID (assuming the totalVotes > 2)
    for (unsigned int i = 0; i < textContours.size(); i++)
    {
      if (textContours.goodIndices[i] == false)
        continue;

      if (totalVotes <= 2)
      {
        textContours.goodIndices[i] = true;
      }
      else if (textContours.hierarchy[i][3] == winningParentId)
      {
        textContours.goodIndices[i] = true;
      }
    }

  }

  void CharacterAnalysis::filterBetweenLines(Mat img, TextContours& textContours, vector<TextLine> textLines )
  {
    static float MIN_AREA_PERCENT_WITHIN_LINES = 0.88;
    static float MAX_DISTANCE_PERCENT_FROM_LINES = 0.15;

    if (textLines.size() == 0)
      return;

    vector<Point> validPoints;


    // Create a white mask for the area inside the polygon
    Mat outerMask = Mat::zeros(img.size(), CV_8U);

    for (unsigned int i = 0; i < textLines.size(); i++)
      fillConvexPoly(outerMask, textLines[i].linePolygon.data(), textLines[i].linePolygon.size(), Scalar(255,255,255));

    // For each contour, determine if enough of it is between the lines to qualify
    for (unsigned int i = 0; i < textContours.size(); i++)
    {
      if (textContours.goodIndices[i] == false)
        continue;

      float percentInsideMask = getContourAreaPercentInsideMask(outerMask, 
              textContours.contours,
              textContours.hierarchy, 
              (int) i);



      if (percentInsideMask < MIN_AREA_PERCENT_WITHIN_LINES)
      {
        // Not enough area is inside the lines.
        if (config->debugCharAnalysis)
          cout << "Rejecting due to insufficient area" << endl;
        textContours.goodIndices[i] = false; 

        continue;
      }


      // now check to make sure that the top and bottom of the contour are near enough to the lines

      // First get the high and low point for the contour
      // Remember that origin is top-left, so the top Y values are actually closer to 0.
      Rect brect = boundingRect(textContours.contours[i]);
      int xmiddle = brect.x + (brect.width / 2);
      Point topMiddle = Point(xmiddle, brect.y);
      Point botMiddle = Point(xmiddle, brect.y+brect.height);

      // Get the absolute distance from the top and bottom lines

      for (unsigned int i = 0; i < textLines.size(); i++)
      {
        Point closestTopPoint = textLines[i].topLine.closestPointOnSegmentTo(topMiddle);
        Point closestBottomPoint = textLines[i].bottomLine.closestPointOnSegmentTo(botMiddle);

        float absTopDistance = distanceBetweenPoints(closestTopPoint, topMiddle);
        float absBottomDistance = distanceBetweenPoints(closestBottomPoint, botMiddle);

        float maxDistance = textLines[i].lineHeight * MAX_DISTANCE_PERCENT_FROM_LINES;

        if (absTopDistance < maxDistance && absBottomDistance < maxDistance)
        {
          // It's ok, leave it as-is.
        }
        else
        {

          textContours.goodIndices[i] = false; 
          if (config->debugCharAnalysis)
            cout << "Rejecting due to top/bottom points that are out of range" << endl;
        }
      }

    }

  }

  void CharacterAnalysis::filterByOuterMask(TextContours& textContours)
  {
    float MINIMUM_PERCENT_LEFT_AFTER_MASK = 0.1;
    float MINIMUM_PERCENT_OF_CHARS_INSIDE_PLATE_MASK = 0.6;

    if (this->pipeline_data->hasPlateBorder == false)
      return;


    cv::Mat plateMask = pipeline_data->plateBorderMask;

    Mat tempMaskedContour = Mat::zeros(plateMask.size(), CV_8U);
    Mat tempFullContour = Mat::zeros(plateMask.size(), CV_8U);

    int charsInsideMask = 0;
    int totalChars = 0;

    vector<bool> originalindices;
    for (unsigned int i = 0; i < textContours.size(); i++)
      originalindices.push_back(textContours.goodIndices[i]);

    for (unsigned int i=0; i < textContours.size(); i++)
    {
      if (textContours.goodIndices[i] == false)
        continue;

      totalChars++;
      tempFullContour = Mat::zeros(plateMask.size(), CV_8U);
      drawContours(tempFullContour, textContours.contours, i, Scalar(255,255,255), CV_FILLED, 8, textContours.hierarchy);
      bitwise_and(tempFullContour, plateMask, tempMaskedContour);
      
      textContours.goodIndices[i] = false;

      float beforeMaskWhiteness = mean(tempFullContour)[0];
      float afterMaskWhiteness = mean(tempMaskedContour)[0];

      if (afterMaskWhiteness / beforeMaskWhiteness > MINIMUM_PERCENT_LEFT_AFTER_MASK)
      {
        charsInsideMask++;
        textContours.goodIndices[i] = true;
      }
    }

    if (totalChars == 0)
    {
      textContours.goodIndices = originalindices;
      return;
    }

    // Check to make sure that this is a valid box.  If the box is too small (e.g., 1 char is inside, and 3 are outside)
    // then don't use this to filter.
    float percentCharsInsideMask = ((float) charsInsideMask) / ((float) totalChars);
    if (percentCharsInsideMask < MINIMUM_PERCENT_OF_CHARS_INSIDE_PLATE_MASK)
    {
      textContours.goodIndices = originalindices;
      return;
    }

  }

  bool CharacterAnalysis::isPlateInverted()
  {
    Mat charMask = getCharacterMask();


    Scalar meanVal = mean(bestThreshold, charMask)[0];

    if (this->config->debugCharAnalysis)
      cout << "CharacterAnalysis, plate inverted: MEAN: " << meanVal << " : " << bestThreshold.type() << endl;

    if (meanVal[0] < 100)		// Half would be 122.5.  Give it a little extra oomf before saying it needs inversion.  Most states aren't inverted.
      return true;

    return false;
  }


  vector<Point> CharacterAnalysis::getCharArea(LineSegment topLine, LineSegment bottomLine)
  {
    const int MAX = 100000;
    const int MIN= -1;

    int leftX = MAX;
    int rightX = MIN;

    for (unsigned int i = 0; i < bestContours.size(); i++)
    {
      if (bestContours.goodIndices[i] == false)
        continue;

      for (unsigned int z = 0; z < bestContours.contours[i].size(); z++)
      {
        if (bestContours.contours[i][z].x < leftX)
          leftX = bestContours.contours[i][z].x;
        if (bestContours.contours[i][z].x > rightX)
          rightX = bestContours.contours[i][z].x;
      }
    }

    vector<Point> charArea;
    if (leftX != MAX && rightX != MIN)
    {
      Point tl(leftX, topLine.getPointAt(leftX));
      Point tr(rightX, topLine.getPointAt(rightX));
      Point br(rightX, bottomLine.getPointAt(rightX));
      Point bl(leftX, bottomLine.getPointAt(leftX));
      charArea.push_back(tl);
      charArea.push_back(tr);
      charArea.push_back(br);
      charArea.push_back(bl);
    }

    return charArea;
  }

}
