#pragma once

#include "stdafx.h"

#include "GrabCut.h"
#include "Superpixel.h"



class DoCutConnect
{
private:

	Mat image;
	Mat spImage;
	vector<int> klabels;			//���صĳ����ر��					��������ͼ��ԭʼ��ǣ����ڶ�ʹ��rect��Χ�ڵ��±��
	int numlabels;					//�������ܸ���

	vector<vector<int> > superarcs;
	vector<Vec6d> superpixels;			//BGRAXY

	vector<vector<int> > spContains;	//ÿ�������ذ�����Щ����

	vector<int> spRelation;			//�¾ɳ����ر�Ƕ�Ӧ��ϵ  spRelation[��/ԭʼ] = spRelation[��]
	int spNum;

	Scalar Superpixels_Contour_Color;
	Scalar Segment_Contour_Color;
	Scalar Rect_Color;
	Scalar Fore_Line_Color;
	Scalar Fore_Pr_Line_Color;
	Scalar Back_Line_Color;
	Scalar Back_Pr_Line_Color;


	Rect rect;
	set<MyPoint> fgdPixels, bgdPixels, prFgdPixels, prBgdPixels;

	vector<int> spMask;		//rect��Χ�ĳ�����mask
	Mat mask;				//rect��Χ������mask
	Mat fullMask;			//����ͼ�������mask

	GMM fgdGMM;
	GMM bgdGMM;
	SuperpixelGrabCut spGrabcut;
	double beta;

	bool isFirstPxlCut;
	SLICO slico;

	PixelsGrabCut pxlGrabcut;

private:
	


public:

	DoCutConnect();
	void setColor(Scalar* fgColor, Scalar* bgColor, Scalar* ctColor);
	void setImage(Mat& inImage);

	double doSuperpixel();
	Mat doDrawArcAndOther();
	void doSuperpixelSegmentation(Rect* _rect, set<MyPoint>* _fgdPixels, set<MyPoint>* _bgdPixels, set<MyPoint>* _prFgdPixels, set<MyPoint>* _prBgdPixels, int iterCount = 1);
	void doPixelSegmentation();






private:

	int getRealSPLabel(int pixelIndex);

	void initSPMask(Rect* _rect);
	void updateSPMask(set<MyPoint>* _fgdPixels, set<MyPoint>* _bgdPixels, set<MyPoint>* _prFgdPixels, set<MyPoint>* _prBgdPixels);

	void prepareLaterGrabCut(vector<Vec6d>& superpixels, vector<vector<int> >& superarcs, vector<int>& spMask);


	void setFullMaskFromSPMask();
	void setFullMaskFromMask();


public:
	Mat getImage(bool isSPImage=true);
	Mat getFullMask();

};

