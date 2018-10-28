#include "opencv2/opencv.hpp"
#include <Windows.h>
#include <gl/GL.h>

#define PI			3.1415926535
#define WIDTH		800
#define HEIGHT		800
#define SCALE_STEP	0.1f

using namespace cv;

/* 2d visualization */
float gScale2d = 1;

Point gTargetPoint;
Rect gRoiRect2d(0, 0, WIDTH, HEIGHT);

Mat gSrcImg(Size(WIDTH, HEIGHT), CV_8UC3, Scalar(100, 100, 100));
Mat gScaleImg(Size(WIDTH, HEIGHT), CV_8UC3, Scalar(100, 100, 100));
Mat gResultImg(Size(WIDTH, HEIGHT), CV_8UC3, Scalar(100, 100, 100));

/**
  * 绘制十字
  * @param[in] img 目标图像
  * @param[in] point 十字中心点
  * @param[in] color 颜色
  * @param[in] size 十字尺寸
  * @param[in] thickness 粗细
  */
void drawCross(Mat img, CvPoint point, CvScalar color, int size, int thickness)
{
	//绘制横线
	line(img, Point(point.x - size / 2, point.y), Point(point.x + size / 2, point.y), color, thickness, 8, 0);
	//绘制竖线
	line(img, Point(point.x, point.y - size / 2), Point(point.x, point.y + size / 2), color, thickness, 8, 0);
}

void Update2d()
{
	resize(gSrcImg, gScaleImg, Size(gSrcImg.cols*gScale2d, gSrcImg.rows*gScale2d), 0, 0, InterpolationFlags::INTER_AREA);

	Mat transformMat = Mat::zeros(2, 3, CV_32FC1);		//定义仿射变形矩阵
	transformMat.at<float>(0, 0) = 1;
	transformMat.at<float>(0, 2) = gRoiRect2d.x; //水平平移量
	transformMat.at<float>(1, 1) = 1;
	transformMat.at<float>(1, 2) = gRoiRect2d.y; //竖直平移量

	warpAffine(gScaleImg, gResultImg, transformMat, gResultImg.size());//根据平移矩阵进行仿射变换

	char text[128];
	sprintf(text, "ROI RECT X = %d, Y = %d", gRoiRect2d.x, gRoiRect2d.y);
	putText(gResultImg, text, cv::Point(50, 50), cv::FONT_HERSHEY_COMPLEX, 0.5, Scalar(0, 255, 255));

	imshow("2d visualizer", gResultImg);
}
void OnMouse2d(int event, int x, int y, int flags, void* userdata)
{
	if (x < 0 || x > WIDTH - 1 || y < 0 || y > HEIGHT - 1)
		return;

	static int startRoiX = 0;
	static int startRoiY = 0;
	static Point startPoint = Point(0, 0);

	//左键按下，记录开始移动时的位置
	if (event == CV_EVENT_LBUTTONDOWN) {
		startPoint = Point(x, y);
		startRoiX = gRoiRect2d.x;
		startRoiY = gRoiRect2d.y;
	}

	//左键按下且鼠标移动中，记录移动的距离
	if (event == CV_EVENT_MOUSEMOVE && (flags & CV_EVENT_LBUTTONDOWN)) {
		int dx = x - startPoint.x;
		int dy = y - startPoint.y;
		gRoiRect2d.x = startRoiX + dx;
		gRoiRect2d.y = startRoiY + dy;
	}

	//右键按下，在图像中画一个点
	if (event == CV_EVENT_RBUTTONDOWN) {
		gTargetPoint.x = (float)(x - gRoiRect2d.x) / gScale2d;
		gTargetPoint.y = (float)(y - gRoiRect2d.y) / gScale2d;
		circle(gSrcImg, Point(gTargetPoint.x, gTargetPoint.y), 1, Scalar(0, 0, 255), 4);
	}

	//滚轮滚动，对图像进行缩放
	if (event == CV_EVENT_MOUSEWHEEL) {
		int value = getMouseWheelDelta(flags);
		float scaleStep = 0;
		if (value > 0)
			scaleStep = SCALE_STEP;
		else if (value < 0)
			scaleStep = -SCALE_STEP;
		gScale2d *= (1 + scaleStep);
		gRoiRect2d.x = x + (gRoiRect2d.x - x)*(1 + scaleStep);
		gRoiRect2d.y = y + (gRoiRect2d.y - y)*(1 + scaleStep);
	}

	Update2d();
}

int main()
{
	namedWindow("2d visualizer", CV_WINDOW_AUTOSIZE);
	setMouseCallback("2d visualizer", OnMouse2d);

	while (true) {
		waitKey(2);
	}

	return 0;
}