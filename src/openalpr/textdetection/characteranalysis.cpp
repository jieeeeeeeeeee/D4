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
  //ȥ����ֵͼ���Ե��ͻ����
  //uthreshold��vthreshold�ֱ��ʾͻ�����Ŀ����ֵ�͸߶���ֵ
  //type����ͻ��������ɫ��0��ʾ��ɫ��1�����ɫ 
  void delete_jut(Mat& src, Mat& dst, int uthreshold, int vthreshold, int type)
  {
	  int threshold;
	  src.copyTo(dst);
	  int height = dst.rows;
	  int width = dst.cols;
	  int k;  //����ѭ���������ݵ��ⲿ
	  for (int i = 0; i < height - 1; i++)
	  {
		  uchar* p = dst.ptr<uchar>(i);
		  for (int j = 0; j < width - 1; j++)
		  {
			  if (type == 0)
			  {
				  //������
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
				  //������
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
				  //������
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
				  //������
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
  //ͼƬ��Ե�⻬����
  //size��ʾȡ��ֵ�Ĵ��ڴ�С��threshold��ʾ�Ծ�ֵͼ����ж�ֵ������ֵ
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


  //ȥ�������Ϸ���ť��
  //����ÿ��Ԫ�صĽ�Ծ�������С��X��Ϊ��������������ȫ����0��Ϳ�ڣ�
  // X���Ƽ�ֵΪ���ɸ���ʵ�ʵ���
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
			  //�����ַ�����һ����������
			  iCount++;
		  }
	  }

	  ////�����Ĳ��ǳ���
	  if (iCount * 1.0 / img.rows <= 0.40) {
		  //�������������������ҲҪ��һ������ֵ��
		  return false;
	  }
	  //�����㳵�Ƶ�����
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
	//����3����ֵ��ͼ��
    pipeline_data->thresholds = produceThresholds(pipeline_data->crop_gray, config);

	//q modify
	//Ϊ�˵õ��ַ��Ƿ���Ҫinvert����֮ǰ�Ĵ���Ľ����Ϊһ��Ԥ����
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
		  //�˲� 
		  //���룺��ֵ��Ӱ�����������
		  //������õ��ַ�����
		  this->filter(pipeline_data->thresholds[i], allTextContours[i]);

		  if (config->debugCharAnalysis)
			cout << "Threshold " << i << " had " << allTextContours[i].getGoodIndicesCount() << " good indices." << endl;
		}

		//��úõ��ַ���������ģ
		PlateMask plateMask(pipeline_data);

		plateMask.findOuterBoxMask(allTextContours);

		pipeline_data->hasPlateBorder = plateMask.hasPlateMask;
		pipeline_data->plateBorderMask = plateMask.getMask();


		if (plateMask.hasPlateMask)
		{
		  // Filter out bad contours now that we have an outer box mask...
		  for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
		  {
			//�����ַ�������ģ���˲���úõ�������ַ�
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
			//��õĶ�ֵ��ͼ��
			bestThreshold = pipeline_data->thresholds[i];
			//��õ��ַ�����
			bestContours = allTextContours[i];
		  }
		}
	}

	//q modify


	//q modify
	//ע�͵��˲���
	//pipeline_data->plate_inverted = true��������Ҫת�����ַ���Ҫ��ף�����bsetbox�ǻ��ڰ�ɫ����
	//��Ϊǰ���Ѿ��жϹ�һ���Ƿ��ַ���Ҫ����ˣ��Ҹı��˶�ֵ��ͼ�����Դ˴�����Ҫ���ж��ˣ��ж�һ�ξ͹��ˣ�
	//�͹��Բü������ͼ�Ƿ���ж�ֵ���������ˣ�bitwise_not��
	//����֤���ұ߿�ͺ�������ַ���ʹ�ð�ɫ��
	//ʶ������͵�ɫ�Ķ�ֵͼ����֤�����Ƕ�ֵͼ�İ�ɫ

    if (config->auto_invert)
      pipeline_data->plate_inverted = isPlateInverted();
    else
      pipeline_data->plate_inverted = config->always_invert;

	//q modify
    if (config->debugGeneral)
      cout << "Plate inverted: " << pipeline_data->plate_inverted << endl;
    
	//q modify
	allTextContours.clear();
	//���ַ�������һ��ƽ��
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
		//�˲� 
		//���룺��ֵ��Ӱ�����������
		//������õ��ַ�����
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

	//��úõ��ַ���������ģ
	PlateMask plateMask(pipeline_data);

	plateMask.findOuterBoxMask(allTextContours);

	pipeline_data->hasPlateBorder = plateMask.hasPlateMask;
	pipeline_data->plateBorderMask = plateMask.getMask();


	if (plateMask.hasPlateMask)
	{
		// Filter out bad contours now that we have an outer box mask...
		for (unsigned int i = 0; i < pipeline_data->thresholds.size(); i++)
		{
			//�����ַ�������ģ���˲���úõ�������ַ�

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
			//��õĶ�ֵ��ͼ��
			bestThreshold = pipeline_data->thresholds[i];
			//��õ��ַ�����
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

	//��ʾ�˲�������е��ַ���������ɫΪ�õģ���ɫΪ���
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
	//�õ���չ����ߣ������˫�У���������
    vector<vector<Point> > linePolygons = lf.findLines(pipeline_data->crop_gray, bestContours);
	//TextLine���ַ��ߣ��������� ��չ�ߣ���ѡ�ַ��ߣ���ѡ�ַ����Σ��ַ�ƽ���߶ȣ����ߵĽǶȵ���
    vector<TextLine> tempTextLines;
    for (unsigned int i = 0; i < linePolygons.size(); i++)
    {
      vector<Point> linePolygon = linePolygons[i];

      LineSegment topLine = LineSegment(linePolygon[0].x, linePolygon[0].y, linePolygon[1].x, linePolygon[1].y);
      LineSegment bottomLine = LineSegment(linePolygon[3].x, linePolygon[3].y, linePolygon[2].x, linePolygon[2].y);
	  //�õ���ѡ�ַ��ķ�Χ�����
      vector<Point> textArea = getCharArea(topLine, bottomLine);

      TextLine textLine(textArea, linePolygon, pipeline_data->crop_gray.size());

      tempTextLines.push_back(textLine);
    }
	//������ѡ�����ٶ�bestThreshold�����˲�
    filterBetweenLines(bestThreshold, bestContours, tempTextLines);

    // Sort the lines from top to bottom.
    std::sort(tempTextLines.begin(), tempTextLines.end(), sort_text_line);

    // Now that we've filtered a few more contours, re-do the text area.
	//�õ��ٴ��˲����textLines
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
	//��ѡ����ַ�Ӧ���������߸����������Χ�е�y���е������ڸߵ�0.3-0.7�ķ�Χ��
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

	//��Ӿ��β����غ�
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






	//�㷨2���������ڷ�
	//1.��ƽ������ѡ�ַ��Ĵ�С��������һ����ѡ����A
	//2.������ߵ������ַ���ʼ����A�أ������ַ��ǵ����߻���
	//3.����򵽵���������С������B�����B�Ĵ�С������A������Ϊ�ǿ��˺��֡���Ϊ��ѡ�ַ���
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
	//�ں�����ַ�
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


	//�ں��ұ��ַ�
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


	//������ѡ���ƽ����С   �ҵ��������е����������ʸ���
	//�Ҹ�������Ȼ�󻭸�ƽ�����Σ������Ӿ������ڷ����ڣ��ͼ������У�Ȼ���ټ��������Σ����㵽һ����break�ˣ�Ч������
	//����ȷ����30������24
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
