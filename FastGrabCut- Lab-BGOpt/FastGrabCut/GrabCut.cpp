#include "GrabCut.h"

Vec3d SuperpixelGrabCut::Vec6d_to_Vec3d(Vec6d v6)
{
	Vec3d v3;
	v3[0] = v6[0];
	v3[1] = v6[1];
	v3[2] = v6[2];
	return v3;
}
Vec2d SuperpixelGrabCut::Vec6d_to_Vec2d(Vec6d v6)
{
	Vec2d v2;
	v2[0] = v6[4];
	v2[1] = v6[5];
	return v2;
}

//����beta��Ҳ����Gibbs�������еĵڶ��ƽ����е�ָ�����beta����������
//�߻��ߵͶԱȶ�ʱ�������������صĲ���Ӱ��ģ������ڵͶԱȶ�ʱ����������
//���صĲ����ܾͻ�Ƚ�С����ʱ����Ҫ����һ���ϴ��beta���Ŵ�������
//�ڸ߶Աȶ�ʱ������Ҫ��С����ͱȽϴ�Ĳ��
//����������Ҫ��������ͼ��ĶԱȶ���ȷ������beta������ļ����Ĺ�ʽ��5����
/*
  Calculate beta - parameter of GrabCut algorithm.
  beta = 1/(2*avg(sqr(||color[i] - color[j]||)))
*/

double SuperpixelGrabCut::calcBeta( const Mat& img )
{
    double beta = 0;
    for( int y = 0; y < img.rows; y++ )
    {
        for( int x = 0; x < img.cols; x++ )
        {
			//�ۻ������������ز�ĵ��
			//�����ĸ��������������صĲ��Ҳ����ŷʽ�������˵���׷���   ֻ����ͼ��������һ�루left upleft up upright������ֹ�ظ�����
			//�����������ض�����󣬾��൱�ڼ������������ز��ˣ�
            Vec3d color = img.at<Vec3b>(y,x);
            if( x>0 ) // color-left  >0���ж���Ϊ�˱�����ͼ��߽��ʱ�򻹼��㣬����Խ��
            {
                Vec3d diff = color - (Vec3d)img.at<Vec3b>(y,x-1);
                beta += diff.dot(diff);  //����ĵ�ˣ�Ҳ���Ǹ���Ԫ��ƽ���ĺ�
            }
            if( y>0 && x>0 ) // color-upleft
            {
                Vec3d diff = color - (Vec3d)img.at<Vec3b>(y-1,x-1);
                beta += diff.dot(diff);
            }
            if( y>0 ) // color-up
            {
                Vec3d diff = color - (Vec3d)img.at<Vec3b>(y-1,x);
                beta += diff.dot(diff);
            }
            if( y>0 && x<img.cols-1) // color-upright
            {
                Vec3d diff = color - (Vec3d)img.at<Vec3b>(y-1,x+1);
                beta += diff.dot(diff);
            }
        }
    }
    if( beta <= std::numeric_limits<double>::epsilon() )
        beta = 0;
    else
        beta = 1.f / (2 * beta/(4*img.cols*img.rows - 3*img.cols - 3*img.rows + 2) ); //���Ĺ�ʽ��5��   beta= 1.0f / (2 * beta/(֮ǰ�ۻ���˵��������صı���));   ����ͼ8����

    return beta;
}

//����ͼÿ���Ƕ˵㶥�㣨Ҳ����ÿ��������Ϊͼ��һ�����㣬������Դ��s�ͻ��t�������򶥵�
//�ıߵ�Ȩֵ������������ͼ�����Ǽ�����ǰ�������ô����һ�����㣬���Ǽ����ĸ�������У�
//�������Ķ�������ʱ�򣬻��ʣ�����ĸ������Ȩֵ�����������������ͼ�����ÿ������
//�������Ķ���ıߵ�Ȩֵ�Ͷ���������ˡ�
//����൱�ڼ���Gibbs�����ĵڶ��������ƽ���������������й�ʽ��4��  �����й�ʽ(11)
/*
  Calculate weights of noterminal vertices of graph.
  beta and gamma - parameters of GrabCut algorithm.
 */

void SuperpixelGrabCut::calcNWeights(vector<Vec6d>& superpixels, vector<vector<int> >& superarcs, vector<vector<double> >& spNWeights, int& sideNum, double beta, double gamma)
{
    //gammaDivSqrt2�൱�ڹ�ʽ��4���е�gamma * dis(i,j)^(-1)����ô����֪����
	//��i��j�Ǵ�ֱ����ˮƽ��ϵʱ��dis(i,j)=1�����ǶԽǹ�ϵʱ��dis(i,j)=sqrt(2.0f)��
	//�������ʱ���������������
	spNWeights.resize(superpixels.size());

	for(int i = 0; i < superarcs.size(); i++)
	{
		spNWeights[i].resize(superarcs[i].size(), 0);
		for(int j = 0; j < superarcs[i].size(); j++)
		{

			//if(i >= superarcs[i][j]) continue;
			int start = i;
			int end = superarcs[i][j];

			Vec3d diff = Vec6d_to_Vec3d(superpixels[start]) - Vec6d_to_Vec3d(superpixels[end]);
			Vec2d dist2d = Vec6d_to_Vec2d(superpixels[start]) - Vec6d_to_Vec2d(superpixels[end]);
			int spSizeStart = superpixels[start][3];	//�����سߴ磨�����ذ��������ظ�����
			int spSizeEnd = superpixels[end][3];
			double sizeRate = (double)spSizeEnd / spSizeStart;	//�����سߴ����
			double dist = std::sqrt(dist2d.dot(dist2d));

			spNWeights[i][j] = exp(-beta*diff.dot(diff)) * (gamma * sizeRate / dist);	//i�ų����ص����j���ھ�(�ھӱ����superarcs[i][j]�õ�)��Ȩֵ

		}
	}

}

//���mask����ȷ�ԡ�maskΪͨ���û��������߳����趨�ģ����Ǻ�ͼ���Сһ���ĵ�ͨ���Ҷ�ͼ��
//ÿ������ֻ��ȡGC_BGD or GC_FGD or GC_PR_BGD or GC_PR_FGD ����ö��ֵ���ֱ��ʾ������
//���û����߳���ָ�������ڱ�����ǰ��������Ϊ�������߿���Ϊǰ�����ء�����Ĳο���
//ICCV2001��Interactive Graph Cuts for Optimal Boundary & Region Segmentation of Objects in N-D Images��
//Yuri Y. Boykov Marie-Pierre Jolly 
/*
  Check size, type and element values of mask matrix.
 */
void SuperpixelGrabCut::checkMask(const vector<Vec6d>& superpixels, const vector<int>& spMask )
{
	if(spMask.size() != superpixels.size())
		CV_Error( CV_StsBadArg, "mask must have as many rows and cols as img" );

	for(int i = 0; i < spMask.size(); i++)
	{
		if(spMask[i] != GC_BGD && spMask[i] != GC_FGD && spMask[i] != GC_PR_BGD && spMask[i] != GC_PR_FGD)
			CV_Error( CV_StsBadArg, "mask element value must be equel"
                    "GC_BGD or GC_FGD or GC_PR_BGD or GC_PR_FGD" );
	}

}

//ͨ���û���ѡĿ��rect������mask��rect���ȫ����Ϊ����������ΪGC_BGD��
//rect�ڵ�����Ϊ GC_PR_FGD������Ϊǰ����
/*
  Initialize mask using rectangular.
*/
void SuperpixelGrabCut::initMaskWithRect(vector<int>& klabels, int width, vector<int>& mask, int size, Rect rect )
{
	mask.resize(size, GC_BGD);
	for(int r = 0; r < rect.height; r++)
	{
		for(int c = 0; c < rect.width; c++)
		{
			int index = (r+rect.y) * width + (c+rect.x);
			int rIndex = r * rect.width + c;
			mask[klabels[index]] = GC_PR_FGD;
		}
	}

}



//ͨ��k-means�㷨����ʼ������GMM��ǰ��GMMģ��
/*
  Initialize GMM background and foreground models using kmeans algorithm.
*/
void SuperpixelGrabCut::initGMMs(vector<Vec6d>& superpixels, vector<vector<int> >& superarcs, vector<int>& spMask, GMM& bgdGMM, GMM& fgdGMM )
{
    const int kMeansItCount = 10;  //��������
    const int kMeansType = KMEANS_PP_CENTERS; //Use kmeans++ center initialization by Arthur and Vassilvitskii

	//��¼������ǰ����������������ÿ�����ض�ӦGMM���ĸ���˹ģ�ͣ������е�kn
	Mat _bgdLabels, _fgdLabels;
	vector<Vec3f> bgdSamples, fgdSamples;//������ǰ��������������

	for(int i = 0; i < spMask.size(); i++)
	{
		if(spMask[i] == GC_BGD || spMask[i] == GC_PR_BGD)
			bgdSamples.push_back((Vec3f)Vec6d_to_Vec3d(superpixels[i]));
		else 
			fgdSamples.push_back((Vec3f)Vec6d_to_Vec3d(superpixels[i]));
	}

	//kmeans�в���_bgdSamplesΪ��ÿ��һ������
	//kmeans�����ΪbgdLabels�����汣�����������������ÿһ��������Ӧ�����ǩ��������ΪcomponentsCount���

	Mat _bgdSamples( (int)bgdSamples.size(), 3, CV_32FC1, &bgdSamples[0][0] );
    kmeans( _bgdSamples, GMM::componentsCount, _bgdLabels,
            TermCriteria( CV_TERMCRIT_ITER, kMeansItCount, 0.0), 0, kMeansType );
    Mat _fgdSamples( (int)fgdSamples.size(), 3, CV_32FC1, &fgdSamples[0][0] );
    kmeans( _fgdSamples, GMM::componentsCount, _fgdLabels,
            TermCriteria( CV_TERMCRIT_ITER, kMeansItCount, 0.0), 0, kMeansType );


    //��������Ĳ����ÿ�����������ĸ�˹ģ�;�ȷ�����ˣ���ô�Ϳ��Թ���GMM��ÿ����˹ģ�͵Ĳ����ˡ�
	bgdGMM.initLearning();	//��ʼ��ĳЩ����=0
    for( int i = 0; i < (int)bgdSamples.size(); i++ )
        bgdGMM.addSample( _bgdLabels.at<int>(i,0), bgdSamples[i] );
    bgdGMM.endLearning();	//������� �� ����ĳЩ�������Ϣ

    fgdGMM.initLearning();
    for( int i = 0; i < (int)fgdSamples.size(); i++ )
        fgdGMM.addSample( _fgdLabels.at<int>(i,0), fgdSamples[i] );
    fgdGMM.endLearning();
}

//�����У�������С���㷨step 1��Ϊÿ�����ط���GMM�������ĸ�˹ģ�ͣ�kn������Mat compIdxs�У�û������ǰ�����Ǳ�����ĸ�˹����������ͨ��mask�ж�ǰ�����Ǳ�����
/*
  Assign GMMs components for each pixel.
*/
void SuperpixelGrabCut::assignGMMsComponents(vector<Vec6d>& superpixels, vector<int>& spMask, GMM& bgdGMM, GMM& fgdGMM, vector<int>& compIdxs)
{
	for(int i = 0; i < spMask.size(); i++)
	{
		if(spMask[i] == GC_BGD || spMask[i] == GC_PR_BGD)
			compIdxs[i] = bgdGMM.whichComponent(Vec6d_to_Vec3d(superpixels[i]));
		else
			compIdxs[i] = fgdGMM.whichComponent(Vec6d_to_Vec3d(superpixels[i]));
	}
}

//�����У�������С���㷨step 2����ÿ����˹ģ�͵�������������ѧϰÿ����˹ģ�͵Ĳ���
/*
  Learn GMMs parameters.
*/

void SuperpixelGrabCut::learnGMMs( vector<Vec6d>& superpixels, vector<int>& spMask, vector<int>& compIdxs,  GMM& bgdGMM, GMM& fgdGMM )
{
	//��ر�����ʼ��=0
    bgdGMM.initLearning();
    fgdGMM.initLearning();

	//�������ص�ǰ������������
	for(int ci = 0; ci < GMM::componentsCount; ci++)
	{
		for(int i = 0; i < spMask.size(); i++)
		{
			if(compIdxs[i] == ci)
			{
				if(spMask[i] == GC_BGD || spMask[i] == GC_PR_BGD)
					bgdGMM.addSample(ci, Vec6d_to_Vec3d(superpixels[i]));
				else 
					fgdGMM.addSample(ci, Vec6d_to_Vec3d(superpixels[i]));
			}
		}
	}

	//����ģ�Ͳ��� ��ĳЩ�������Ϣ
    bgdGMM.endLearning();
    fgdGMM.endLearning();
}

//ͨ������õ����������ͼ��ͼ�Ķ���Ϊ���ص㣬ͼ�ı��������ֹ��ɣ�
//һ����ǣ�ÿ��������Sink���t������������Դ��Source������ǰ�������ӵıߣ�
//����ߵ�Ȩֵͨ��Gibbs������ĵ�һ������������ʾ��
//��һ����ǣ�ÿ�������������򶥵����ӵıߣ�����ߵ�Ȩֵͨ��Gibbs������ĵڶ�������������ʾ��
//lambda t-linkȨ��=[0,lambda]
/*
  Construct GCGraph
*/
void SuperpixelGrabCut::constructGCGraph(vector<Vec6d>& superpixels, vector<int>& spMask, GMM& bgdGMM, GMM& fgdGMM, double lambda, vector<vector<int> >& superarcs, vector<vector<double> >& spNWeights, vector<int>& vtxs, int sideNum, GCGraph<double>& graph)
{
	int vtxCount = superpixels.size();
	int edgeCount = 2*sideNum;
	graph.create(vtxCount, edgeCount);

	vtxs.resize(vtxCount);
	for(int i = 0; i < vtxCount; i++)
	{
		// add node
		int vtxIdx = graph.addVtx();  //�������������ͼ�е�����
		vtxs[i] = vtxIdx;
		Vec3b color = Vec6d_to_Vec3d(superpixels[i]);

		//t-link
		double fromSource, toSink;
		if( spMask[i] == GC_PR_BGD || spMask[i] == GC_PR_FGD )
		{
			//double sizeRate = (double)superpixels[i][3] / (10*10);

			//��ÿһ�����ؼ�������Ϊ�������ػ���ǰ�����صĵ�һ���������Ϊ�ֱ���t��s�������Ȩֵ
			fromSource = -log( bgdGMM(color) );	//-log(�������ڱ���ģ�͸���)
			toSink = -log( fgdGMM(color) );		//-log(��������ǰ��ģ�͸���)
		}
		else if( spMask[i] == GC_BGD )
		{
			//����ȷ��Ϊ���������ص㣬����Source�㣨ǰ����������Ϊ0����Sink�������Ϊlambda
			fromSource = 0;
			toSink = lambda;
		}
		else // GC_FGD
		{
			fromSource = lambda;
			toSink = 0;
		}
		//���øö���vtxIdx�ֱ���Source���Sink�������Ȩֵ  t-link
		graph.addTermWeights( vtxIdx, fromSource, toSink );

	}


	// set n-weights  n-links
	//�����������򶥵�֮�����ӵ�Ȩֵ��
	//Ҳ������Gibbs�����ĵڶ��������ƽ���
	vector<int> oneLine(spNWeights.size(), 0);
	vector<vector<int> > allWeights(spNWeights.size(), oneLine);
	for(int i = 0; i < spNWeights.size(); i++)
	{
		for(int j = 0; j < spNWeights[i].size(); j++)
		{
			int start = i;
			int end = superarcs[i][j];
			allWeights[start][end] = spNWeights[i][j];
		}
	}

	for(int i = 0; i < spNWeights.size(); i++)
	{
		for(int j = 0; j < spNWeights[i].size(); j++)
		{
			int start = i;
			int end = superarcs[i][j];
			if(start >= end) continue;
			int valueStart = allWeights[start][end];
			int valueEnd = allWeights[end][start];
			graph.addEdges(vtxs[start], vtxs[end], valueStart, valueEnd);
		}
	}


}

//�����У�������С���㷨step 3���ָ���ƣ���С�����������㷨
/*
  Estimate segmentation using MaxFlow algorithm
*/
void SuperpixelGrabCut::estimateSegmentation( GCGraph<double>& graph, vector<int>& spMask, vector<int> vtxs)
{
    //ͨ��������㷨ȷ��ͼ����С�Ҳ�����ͼ��ķָ�
	graph.maxFlow();

	for(int i = 0; i < spMask.size(); i++)
	{
		//ֻ����PR���
		if(spMask[i] == GC_PR_BGD || spMask[i] == GC_PR_FGD)
		{
			if( graph.inSourceSegment(vtxs[i]) )
				spMask[i] = GC_PR_FGD;
			else
				spMask[i] = GC_PR_BGD;
		}
	}

}

//���ĳɹ����ṩ�����ʹ�õ�ΰ���API��grabCut 
/*
****����˵����
	img�������ָ��Դͼ�񣬱�����8λ3ͨ����CV_8UC3��ͼ���ڴ���Ĺ����в��ᱻ�޸ģ�
	mask��������ͼ�����ʹ��������г�ʼ������ômask�����ʼ��������Ϣ����ִ�зָ�
		��ʱ��Ҳ���Խ��û��������趨��ǰ���뱳�����浽mask�У�Ȼ���ٴ���grabCut��
		�����ڴ������֮��mask�лᱣ������maskֻ��ȡ��������ֵ��
		GCD_BGD��=0����������
		GCD_FGD��=1����ǰ����
		GCD_PR_BGD��=2�������ܵı�����
		GCD_PR_FGD��=3�������ܵ�ǰ����
		���û���ֹ����GCD_BGD����GCD_FGD����ô���ֻ����GCD_PR_BGD��GCD_PR_FGD��
	rect���������޶���Ҫ���зָ��ͼ��Χ��ֻ�иþ��δ����ڵ�ͼ�񲿷ֲű�����
	bgdModel��������ģ�ͣ����Ϊnull�������ڲ����Զ�����һ��bgdModel��bgdModel������
		��ͨ�������ͣ�CV_32FC1��ͼ��������ֻ��Ϊ1������ֻ��Ϊ13x5��  �����и�˹���ģ�͵����в���
	fgdModel����ǰ��ģ�ͣ����Ϊnull�������ڲ����Զ�����һ��fgdModel��fgdModel������
		��ͨ�������ͣ�CV_32FC1��ͼ��������ֻ��Ϊ1������ֻ��Ϊ13x5��
	iterCount���������������������0��
	mode��������ָʾgrabCut��������ʲô��������ѡ��ֵ�У�
		GC_INIT_WITH_RECT��=0�����þ��δ���ʼ��GrabCut��
		GC_INIT_WITH_MASK��=1����������ͼ���ʼ��GrabCut��
		GC_EVAL��=2����ִ�зָ
*/
void SuperpixelGrabCut::grabCut_ForSuperpixels(vector<int>& klabels, vector<Vec6d> superpixels, vector<vector<int> >& superarcs, double beta, vector<int>& spMask,GMM& fgdGMM, GMM& bgdGMM,  int iterCount, int mode)
{
	int spSize = superpixels.size();


	//�������������ڵĸ�˹������û������ǰ�����Ǳ�����ĸ�˹������
	vector<int> compIdxs(spSize,0);

	//���û�û�и������ģ�͵Ĳ���������Ҫѧϰģ�Ͳ�����initGMMs��
	if(mode == GC_INIT_WITH_RECT)	
	{
		return;	//error
	}
    if(mode == GC_INIT_WITH_MASK )
    {//���ηָ�
		//��kmeans++ȷ��ÿ����������ǰ�����Ǳ������ģ���������ĸ���˹������Ȼ������Щ���������ǰ���������ģ�͵����в���
        initGMMs(superpixels, superarcs, spMask, bgdGMM, fgdGMM );
    }


    if( iterCount <= 0)
        return;

	//���û������˻��ģ�Ͳ�������ֻ����£�������(initGMMs)
    if( mode == GC_EVAL )
        checkMask(superpixels, spMask );

    const double gamma = 50;		//������gamma (ϣ����ĸ������)
    const double lambda = 9*gamma;	//������lambda (ϣ����ĸ��ʮһ��)
    //const double beta = calcBeta( img );	//������beta��ϣ����ĸ�ڶ�������������Сͼ�ζԱȶȣ���̫�����С����̫С������

	//����n-link
	vector<vector<double> > spNWeights;
	vector<int> vtxs;			//��¼�����ر����graph�е��ǵĶ�Ӧ��ϵ
	int sideNum = 0;
    //Mat leftW, upleftW, upW, uprightW;
	calcNWeights(superpixels, superarcs, spNWeights, sideNum, beta, gamma);

	//ǰ������kmeans++�㷨ȷ�����ؾ������������ȷ���������ٲ��ܳ�ʼ�����ģ�Ͳ�����Ȼ�����ͼ�бߵ��������������ᷢ���仯��
	//֮������ģ�1.���ݻ��ģ�ͼ�����������������˹������2.���¼�����ģ�Ͳ�����3.����ͼ�4.���������mask��
    for( int i = 0; i < iterCount; i++ )
    {
        GCGraph<double> graph;
		//����ÿ�����������ĸ���˹����
		assignGMMsComponents( superpixels, spMask, bgdGMM, fgdGMM, compIdxs);
		//������ģ�͵����в���/���²�������֮ǰ������Ĳ����в��죩
        learnGMMs( superpixels, spMask, compIdxs, bgdGMM, fgdGMM );
		//����ͼ, ��� t-link n-link
		constructGCGraph(superpixels, spMask, bgdGMM, fgdGMM, lambda, superarcs, spNWeights, vtxs, sideNum, graph);
		//ʹ��������㷨��ɷָȻ�����mask(��Զ����ı��û�ָ���Ĳ���)
        estimateSegmentation(graph, spMask, vtxs);
    }
}




/////////////////////////////



void PixelsGrabCut::calcNWeights( const Mat& img, Mat& leftW, Mat& upleftW, Mat& upW, 
							Mat& uprightW, double beta, double gamma )
{
    //gammaDivSqrt2�൱�ڹ�ʽ��4���е�gamma * dis(i,j)^(-1)����ô����֪����
	//��i��j�Ǵ�ֱ����ˮƽ��ϵʱ��dis(i,j)=1�����ǶԽǹ�ϵʱ��dis(i,j)=sqrt(2.0f)��
	//�������ʱ���������������
	const double gammaDivSqrt2 = gamma / std::sqrt(2.0f);
	//ÿ������ıߵ�Ȩֵͨ��һ����ͼ��С��ȵ�Mat������
    leftW.create( img.rows, img.cols, CV_64FC1 );
    upleftW.create( img.rows, img.cols, CV_64FC1 );
    upW.create( img.rows, img.cols, CV_64FC1 );
    uprightW.create( img.rows, img.cols, CV_64FC1 );
    for( int y = 0; y < img.rows; y++ )
    {
        for( int x = 0; x < img.cols; x++ )
        {
            Vec3d color = img.at<Vec3b>(y,x);
            if( x-1>=0 ) // left  //����ͼ�ı߽�
            {
                Vec3d diff = color - (Vec3d)img.at<Vec3b>(y,x-1);
                leftW.at<double>(y,x) = gamma * exp(-beta*diff.dot(diff));
            }
            else
                leftW.at<double>(y,x) = 0;
            if( x-1>=0 && y-1>=0 ) // upleft
            {
                Vec3d diff = color - (Vec3d)img.at<Vec3b>(y-1,x-1);
                upleftW.at<double>(y,x) = gammaDivSqrt2 * exp(-beta*diff.dot(diff));
            }
            else
                upleftW.at<double>(y,x) = 0;
            if( y-1>=0 ) // up
            {
                Vec3d diff = color - (Vec3d)img.at<Vec3b>(y-1,x);
                upW.at<double>(y,x) = gamma * exp(-beta*diff.dot(diff));
            }
            else
                upW.at<double>(y,x) = 0;
            if( x+1<img.cols && y-1>=0 ) // upright
            {
                Vec3d diff = color - (Vec3d)img.at<Vec3b>(y-1,x+1);
                uprightW.at<double>(y,x) = gammaDivSqrt2 * exp(-beta*diff.dot(diff));
            }
            else
                uprightW.at<double>(y,x) = 0;
        }
    }
}

//���mask����ȷ�ԡ�maskΪͨ���û��������߳����趨�ģ����Ǻ�ͼ���Сһ���ĵ�ͨ���Ҷ�ͼ��
//ÿ������ֻ��ȡGC_BGD or GC_FGD or GC_PR_BGD or GC_PR_FGD ����ö��ֵ���ֱ��ʾ������
//���û����߳���ָ�������ڱ�����ǰ��������Ϊ�������߿���Ϊǰ�����ء�����Ĳο���
//ICCV2001��Interactive Graph Cuts for Optimal Boundary & Region Segmentation of Objects in N-D Images��
//Yuri Y. Boykov Marie-Pierre Jolly 
/*
  Check size, type and element values of mask matrix.
 */
void PixelsGrabCut::checkMask( const Mat& img, const Mat& mask )
{
    if( mask.empty() )
        CV_Error( CV_StsBadArg, "mask is empty" );
    if( mask.type() != CV_8UC1 )
        CV_Error( CV_StsBadArg, "mask must have CV_8UC1 type" );
    if( mask.cols != img.cols || mask.rows != img.rows )
        CV_Error( CV_StsBadArg, "mask must have as many rows and cols as img" );
    for( int y = 0; y < mask.rows; y++ )
    {
        for( int x = 0; x < mask.cols; x++ )
        {
            uchar val = mask.at<uchar>(y,x);
            if( val!=GC_BGD && val!=GC_FGD && val!=GC_PR_BGD && val!=GC_PR_FGD )
                CV_Error( CV_StsBadArg, "mask element value must be equel"
                    "GC_BGD or GC_FGD or GC_PR_BGD or GC_PR_FGD" );
        }
    }
}

//ͨ���û���ѡĿ��rect������mask��rect���ȫ����Ϊ����������ΪGC_BGD��
//rect�ڵ�����Ϊ GC_PR_FGD������Ϊǰ����
/*
  Initialize mask using rectangular.
*/
void PixelsGrabCut::initMaskWithRect( Mat& mask, Size imgSize, Rect rect )
{
    mask.create( imgSize, CV_8UC1 );
    mask.setTo( GC_BGD );

    rect.x = max(0, rect.x);
    rect.y = max(0, rect.y);
    rect.width = min(rect.width, imgSize.width-rect.x);
    rect.height = min(rect.height, imgSize.height-rect.y);

    (mask(rect)).setTo( Scalar(GC_PR_FGD) );
}

//�����У�������С���㷨step 1��Ϊÿ�����ط���GMM�������ĸ�˹ģ�ͣ�kn������Mat compIdxs�У�û������ǰ�����Ǳ�����ĸ�˹����������ͨ��mask�ж�ǰ�����Ǳ�����
/*
  Assign GMMs components for each pixel.
*/
void PixelsGrabCut::assignGMMsComponents( const Mat& img, const Mat& mask, const GMM& bgdGMM, 
									const GMM& fgdGMM, Mat& compIdxs )
{
    Point p;
    for( p.y = 0; p.y < img.rows; p.y++ )
    {
        for( p.x = 0; p.x < img.cols; p.x++ )
        {
            Vec3d color = img.at<Vec3b>(p);
			//ͨ��mask���жϸ��������ڱ������ػ���ǰ�����أ����ж�������ǰ�����߱���GMM�е��ĸ���˹����
			//�ڼ�����ģ�Ͳ���ʱ��ʹ��kmeans++��������ÿ�����������ĸ�˹������������ͨ�����������ڲ�ͬ��˹������ĸ������жϡ�

            compIdxs.at<int>(p) = mask.at<uchar>(p) == GC_BGD || mask.at<uchar>(p) == GC_PR_BGD ? bgdGMM.whichComponent(color) : fgdGMM.whichComponent(color);


        }
    }
}

//�����У�������С���㷨step 2����ÿ����˹ģ�͵�������������ѧϰÿ����˹ģ�͵Ĳ���
/*
  Learn GMMs parameters.
*/
void PixelsGrabCut::learnGMMs( const Mat& img, const Mat& mask, const Mat& compIdxs, GMM& bgdGMM, GMM& fgdGMM )
{
	//��ر�����ʼ��=0
    bgdGMM.initLearning();
    fgdGMM.initLearning();

	//�������ص�ǰ������������
    Point p;
    for( int ci = 0; ci < GMM::componentsCount; ci++ )
    {
        for( p.y = 0; p.y < img.rows; p.y++ )
        {
            for( p.x = 0; p.x < img.cols; p.x++ )
            {
                if( compIdxs.at<int>(p) == ci )
                {
                    if( mask.at<uchar>(p) == GC_BGD || mask.at<uchar>(p) == GC_PR_BGD )
                        bgdGMM.addSample( ci, img.at<Vec3b>(p) );
                    else
                        fgdGMM.addSample( ci, img.at<Vec3b>(p) );
                }
            }
        }
    }

	//����ģ�Ͳ��� ��ĳЩ�������Ϣ
    bgdGMM.endLearning();
    fgdGMM.endLearning();
}

//ͨ������õ����������ͼ��ͼ�Ķ���Ϊ���ص㣬ͼ�ı��������ֹ��ɣ�
//һ����ǣ�ÿ��������Sink���t������������Դ��Source������ǰ�������ӵıߣ�
//����ߵ�Ȩֵͨ��Gibbs������ĵ�һ������������ʾ��
//��һ����ǣ�ÿ�������������򶥵����ӵıߣ�����ߵ�Ȩֵͨ��Gibbs������ĵڶ�������������ʾ��
//lambda t-linkȨ��=[0,lambda]
/*
  Construct GCGraph
*/
void PixelsGrabCut::constructGCGraph( const Mat& img, const Mat& mask, const GMM& bgdGMM, const GMM& fgdGMM, double lambda,
                       const Mat& leftW, const Mat& upleftW, const Mat& upW, const Mat& uprightW,
                       GCGraph<double>& graph )
{
    int vtxCount = img.cols*img.rows;  //��������ÿһ��������һ������

	//(8������������+���ϡ����¡����ϡ����µ���ͨ��(n-link)��δ������source,sink�������(t-link)) ��������ͼ����������������ͼ��2��
    int edgeCount = 2*(4*vtxCount/*left,upLeft,up,upRight*/ - 3*(img.cols + img.rows)/*�����ı߽�ȱʧ��*/ + 2/*�ظ��ı�*/);  //��������Ҫ����ͼ�߽�ıߵ�ȱʧ
    //ͨ���������ͱ�������ͼ����Щ���������ͺ���������ο�gcgraph.hpp
	graph.create(vtxCount, edgeCount);
    Point p;
    for( p.y = 0; p.y < img.rows; p.y++ )
    {
        for( p.x = 0; p.x < img.cols; p.x++)
        {
            // add node
            int vtxIdx = graph.addVtx();  //�������������ͼ�е�����
            Vec3b color = img.at<Vec3b>(p);

            // set t-weights			
            //����ÿ��������Sink���t������������Դ��Source������ǰ�������ӵ�Ȩֵ��
			//Ҳ������Gibbs������ÿһ�����ص���Ϊ�������ػ���ǰ�����أ��ĵ�һ��������
			double fromSource, toSink;
            if( mask.at<uchar>(p) == GC_PR_BGD || mask.at<uchar>(p) == GC_PR_FGD )
            {
                //��ÿһ�����ؼ�������Ϊ�������ػ���ǰ�����صĵ�һ���������Ϊ�ֱ���t��s�������Ȩֵ
				fromSource = -log( bgdGMM(color) );	//-log(�������ڱ���ģ�͸���)
                toSink = -log( fgdGMM(color) );		//-log(��������ǰ��ģ�͸���)
            }
            else if( mask.at<uchar>(p) == GC_BGD )
            {
                //����ȷ��Ϊ���������ص㣬����Source�㣨ǰ����������Ϊ0����Sink�������Ϊlambda
				fromSource = 0;
                toSink = lambda;
            }
            else // GC_FGD
            {
                fromSource = lambda;
                toSink = 0;
            }
			//���øö���vtxIdx�ֱ���Source���Sink�������Ȩֵ
            graph.addTermWeights( vtxIdx, fromSource, toSink );

            // set n-weights  n-links
            //�����������򶥵�֮�����ӵ�Ȩֵ��
			//Ҳ������Gibbs�����ĵڶ��������ƽ���
			if( p.x>0 )
            {
                double w = leftW.at<double>(p);
                graph.addEdges( vtxIdx, vtxIdx-1, w, w );
            }
            if( p.x>0 && p.y>0 )
            {
                double w = upleftW.at<double>(p);
                graph.addEdges( vtxIdx, vtxIdx-img.cols-1, w, w );
            }
            if( p.y>0 )
            {
                double w = upW.at<double>(p);
                graph.addEdges( vtxIdx, vtxIdx-img.cols, w, w );
            }
            if( p.x<img.cols-1 && p.y>0 )
            {
                double w = uprightW.at<double>(p);
                graph.addEdges( vtxIdx, vtxIdx-img.cols+1, w, w );
            }
        }
    }
}

//�����У�������С���㷨step 3���ָ���ƣ���С�����������㷨
/*
  Estimate segmentation using MaxFlow algorithm
*/
void PixelsGrabCut::estimateSegmentation( GCGraph<double>& graph, Mat& mask )
{
    //ͨ��������㷨ȷ��ͼ����С�Ҳ�����ͼ��ķָ�
	graph.maxFlow();
    Point p;
    for( p.y = 0; p.y < mask.rows; p.y++ )
    {
        for( p.x = 0; p.x < mask.cols; p.x++ )
        {
            //ͨ��ͼ�ָ�Ľ��������mask��������ͼ��ָ�����ע����ǣ���Զ��
			//��������û�ָ��Ϊ��������ǰ��������
			if( mask.at<uchar>(p) == GC_PR_BGD || mask.at<uchar>(p) == GC_PR_FGD )
            {
                if( graph.inSourceSegment( p.y*mask.cols+p.x /*vertex index*/ ) )
                    mask.at<uchar>(p) = GC_PR_FGD;
                else
                    mask.at<uchar>(p) = GC_PR_BGD;
            }
        }
    }
}


double PixelsGrabCut::calcBeta( const Mat& img )
{
    double beta = 0;
    for( int y = 0; y < img.rows; y++ )
    {
        for( int x = 0; x < img.cols; x++ )
        {
			//�ۻ������������ز�ĵ��
			//�����ĸ��������������صĲ��Ҳ����ŷʽ�������˵���׷���   ֻ����ͼ��������һ�루left upleft up upright������ֹ�ظ�����
			//�����������ض�����󣬾��൱�ڼ������������ز��ˣ�
            Vec3d color = img.at<Vec3b>(y,x);
            if( x>0 ) // color-left  >0���ж���Ϊ�˱�����ͼ��߽��ʱ�򻹼��㣬����Խ��
            {
                Vec3d diff = color - (Vec3d)img.at<Vec3b>(y,x-1);
                beta += diff.dot(diff);  //����ĵ�ˣ�Ҳ���Ǹ���Ԫ��ƽ���ĺ�
            }
            if( y>0 && x>0 ) // color-upleft
            {
                Vec3d diff = color - (Vec3d)img.at<Vec3b>(y-1,x-1);
                beta += diff.dot(diff);
            }
            if( y>0 ) // color-up
            {
                Vec3d diff = color - (Vec3d)img.at<Vec3b>(y-1,x);
                beta += diff.dot(diff);
            }
            if( y>0 && x<img.cols-1) // color-upright
            {
                Vec3d diff = color - (Vec3d)img.at<Vec3b>(y-1,x+1);
                beta += diff.dot(diff);
            }
        }
    }
    if( beta <= std::numeric_limits<double>::epsilon() )
        beta = 0;
    else
        beta = 1.f / (2 * beta/(4*img.cols*img.rows - 3*img.cols - 3*img.rows + 2) ); //���Ĺ�ʽ��5��   beta= 1.0f / (2 * beta/(֮ǰ�ۻ���˵��������صı���));   ����ͼ8����

    return beta;
}


//���ĳɹ����ṩ�����ʹ�õ�ΰ���API��grabCut 
/*
****����˵����
	img�������ָ��Դͼ�񣬱�����8λ3ͨ����CV_8UC3��ͼ���ڴ���Ĺ����в��ᱻ�޸ģ�
	mask��������ͼ�����ʹ��������г�ʼ������ômask�����ʼ��������Ϣ����ִ�зָ�
		��ʱ��Ҳ���Խ��û��������趨��ǰ���뱳�����浽mask�У�Ȼ���ٴ���grabCut��
		�����ڴ������֮��mask�лᱣ������maskֻ��ȡ��������ֵ��
		GCD_BGD��=0����������
		GCD_FGD��=1����ǰ����
		GCD_PR_BGD��=2�������ܵı�����
		GCD_PR_FGD��=3�������ܵ�ǰ����
		���û���ֹ����GCD_BGD����GCD_FGD����ô���ֻ����GCD_PR_BGD��GCD_PR_FGD��
	rect���������޶���Ҫ���зָ��ͼ��Χ��ֻ�иþ��δ����ڵ�ͼ�񲿷ֲű�����
	bgdModel��������ģ�ͣ����Ϊnull�������ڲ����Զ�����һ��bgdModel��bgdModel������
		��ͨ�������ͣ�CV_32FC1��ͼ��������ֻ��Ϊ1������ֻ��Ϊ13x5��  �����и�˹���ģ�͵����в���
	fgdModel����ǰ��ģ�ͣ����Ϊnull�������ڲ����Զ�����һ��fgdModel��fgdModel������
		��ͨ�������ͣ�CV_32FC1��ͼ��������ֻ��Ϊ1������ֻ��Ϊ13x5��
	iterCount���������������������0��
	mode��������ָʾgrabCut��������ʲô��������ѡ��ֵ�У�
		GC_INIT_WITH_RECT��=0�����þ��δ���ʼ��GrabCut��
		GC_INIT_WITH_MASK��=1����������ͼ���ʼ��GrabCut��
		GC_EVAL��=2����ִ�зָ
*/
void PixelsGrabCut::grabCut_ForPixel( Mat& img, Mat& mask, Rect& rect, GMM& fgdGMM, GMM& bgdGMM, int iterCount, bool isFirst)
{

    if( img.empty() )
        CV_Error( CV_StsBadArg, "image is empty" );
    if( img.type() != CV_8UC3 )
        CV_Error( CV_StsBadArg, "image mush have CV_8UC3 type" );


	//�������������ڵĸ�˹������û������ǰ�����Ǳ�����ĸ�˹������
    Mat compIdxs( img.size(), CV_32SC1 );

	//if(isFirst)
	//{
	//	//initGMMs( img, mask, bgdGMM, fgdGMM );
	//}

	////���û�û�и������ģ�͵Ĳ���������Ҫѧϰģ�Ͳ�����initGMMs��
 //   if( mode == GC_INIT_WITH_RECT || mode == GC_INIT_WITH_MASK )
 //   {
 //       if( mode == GC_INIT_WITH_RECT )
 //           initMaskWithRect( mask, img.size(), rect );	//rect��=GC_BGD  rect��=GC_PR_FGD
 //       else // flag == GC_INIT_WITH_MASK
 //           checkMask( img, mask );	//���mask��ֵ�ķ�Χ��GC_BGD��GC_FGD, GC_PR_BGD, GC_PR_FGD
	//	//��kmeans++ȷ��ÿ����������ǰ�����Ǳ������ģ���������ĸ���˹������Ȼ������Щ���������ǰ���������ģ�͵����в���
 //       initGMMs( img, mask, bgdGMM, fgdGMM );
 //   }
	//initMaskWithRect( mask, img.size(), rect );



    if( iterCount <= 0)
        return;

	//���û������˻��ģ�Ͳ�������ֻ����£�������(initGMMs)
    checkMask( img, mask );

    const double gamma = 50;		//������gamma (ϣ����ĸ������)
    const double lambda = 9*gamma;	//������lambda (ϣ����ĸ��ʮһ��)
    const double beta = calcBeta( img );	//������beta��ϣ����ĸ�ڶ�������������Сͼ�ζԱȶȣ���̫�����С����̫С������

	//����n-link
    Mat leftW, upleftW, upW, uprightW;
    calcNWeights( img, leftW, upleftW, upW, uprightW, beta, gamma );

	//ǰ������kmeans++�㷨ȷ�����ؾ������������ȷ���������ٲ��ܳ�ʼ�����ģ�Ͳ�����Ȼ�����ͼ�бߵ��������������ᷢ���仯��
	//֮������ģ�1.���ݻ��ģ�ͼ�����������������˹������2.���¼�����ģ�Ͳ�����3.����ͼ�4.���������mask��
    for( int i = 0; i < iterCount; i++ )
    {
        GCGraph<double> graph;

		//Mat mask2 = mask.clone();
		//Point p;
		//int a0 = 0;
		//int a1 = 255/3;
		//int a2 = 255/3*2;
		//int a3 = 255;
		//for(p.y = 0; p.y < mask2.rows; p.y++)
		//{
		//	for(p.x = 0; p.x < mask2.cols; p.x++)
		//	{
		//		if(mask2.at<uchar>(p) == 1)
		//			mask2.at<uchar>(p) = a1;
		//		if(mask2.at<uchar>(p) == 2)
		//			mask2.at<uchar>(p) = a2;
		//		if(mask2.at<uchar>(p) == 3)
		//			mask2.at<uchar>(p) = a3;
		//	}
		//}

		//imshow("fullAA", mask2);



		//double begin = (double)getTickCount();
		////...
		//double end = (double)getTickCount();
		//double time = (end-begin)/getTickFrequency()



		//����ÿ�����������ĸ���˹����
        assignGMMsComponents( img, mask, bgdGMM, fgdGMM, compIdxs );
		//������ģ�͵����в���/���²�������֮ǰ������Ĳ����в��죩
        learnGMMs( img, mask, compIdxs, bgdGMM, fgdGMM );
		//����ͼ, ��� t-link n-link
        constructGCGraph(img, mask, bgdGMM, fgdGMM, lambda, leftW, upleftW, upW, uprightW, graph );
		//ʹ��������㷨��ɷָȻ�����mask(��Զ����ı��û�ָ���Ĳ���)
        estimateSegmentation( graph, mask );
    }
}
