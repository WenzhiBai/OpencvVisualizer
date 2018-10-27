#include "opencv2/opencv.hpp"

using namespace cv;

#define WIDTH 800
#define HEIGHT 800
#define SCALE_STEP 0.1f

float resizeRate = 1;

Point targetPoint;
Rect roiRect(0, 0, WIDTH, HEIGHT);

Mat srcImg(Size(WIDTH, HEIGHT), CV_8UC3, Scalar(100, 100, 100));
Mat scaleImg(Size(WIDTH, HEIGHT), CV_8UC3, Scalar(100, 100, 100));
Mat resultImg(Size(WIDTH, HEIGHT), CV_8UC3, Scalar(100, 100, 100));

void on_mouse(int event, int x, int y, int flags, void* userdata)
{
	if (x < 0 || x > WIDTH - 1 || y < 0 || y > HEIGHT - 1)
		return;

	static int start_x = 0;
	static int start_y = 0;
	static Point ld = Point(0, 0);
	static Point ru = Point(0, 0);

	if (event == CV_EVENT_LBUTTONDOWN) {
		ld = Point(x, y);
		start_x = roiRect.x;
		start_y = roiRect.y;
	}
	if (event == CV_EVENT_MOUSEMOVE && (flags & CV_EVENT_LBUTTONDOWN)) {
		int dx = x - ld.x;
		int dy = y - ld.y;
		roiRect.x = start_x + dx;
		roiRect.y = start_y + dy;
	}
	if (event == CV_EVENT_RBUTTONDOWN) {
		targetPoint.x = (float)(x - roiRect.x) / resizeRate;
		targetPoint.y = (float)(y - roiRect.y) / resizeRate;
		//circle(scaleImg, Point(x, y), 1, Scalar(0, 0, 255), 4);
		circle(srcImg, Point(targetPoint.x, targetPoint.y), 1, Scalar(0, 0, 255), 4);
	}
	if (event == CV_EVENT_MOUSEWHEEL) {
		int value = getMouseWheelDelta(flags);
		float scaleStep = SCALE_STEP;
		if (value > 0)
			scaleStep = SCALE_STEP;
		else if (value < 0)
			scaleStep = -SCALE_STEP;
		resizeRate *= (1 + scaleStep);
		resize(srcImg, scaleImg, Size(srcImg.cols*resizeRate, srcImg.rows*resizeRate), 0, 0, InterpolationFlags::INTER_AREA);
		roiRect.x = x + (roiRect.x - x)*(1 + scaleStep);
		roiRect.y = y + (roiRect.y - y)*(1 + scaleStep);
	}

	Mat t_mat = Mat::zeros(2, 3, CV_32FC1);		//定义平移矩阵
	t_mat.at<float>(0, 0) = 1;
	t_mat.at<float>(0, 2) = roiRect.x; //水平平移量
	t_mat.at<float>(1, 1) = 1;
	t_mat.at<float>(1, 2) = roiRect.y; //竖直平移量

	warpAffine(scaleImg, resultImg, t_mat, resultImg.size());//根据平移矩阵进行仿射变换

	char text[256];
	sprintf(text, "ROI RECT X = %d, Y = %d", roiRect.x, roiRect.y);
	putText(resultImg, text, cv::Point(50, 50), cv::FONT_HERSHEY_COMPLEX, 0.5, Scalar(0, 255, 255));

	imshow("test", resultImg);
}

int main()
{
	namedWindow("test", CV_WINDOW_AUTOSIZE);
	setMouseCallback("test", on_mouse);

	while (true) {
		waitKey(2);
	}

	return 0;
}