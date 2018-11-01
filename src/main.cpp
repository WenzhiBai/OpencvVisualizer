#include "opencv2/opencv.hpp"
#include <Windows.h>
#include <gl/GL.h>
#include <vector>

#define PI						3.1415926535
#define WIDTH					800
#define HEIGHT					800
#define SCALE_STEP				0.1f
#define MAX_LINE_BUFFER_SIZE	128

using namespace cv;

String gWindow2dName = "2d visualizer";
String gWindow3dName = "3d visualizer";

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

	imshow(gWindow2dName, gResultImg);
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

/* 3d visualization */
float gThetaX = 0.0;
float gThetaY = 0.0;
float gScaleFactor = 1.0;

float dx = 0.0;
float dy = 0.0;
float dxOld = 0.0;
float dyOld = 0.0;

struct PointsCloud {
	std::vector<Point3f> points;
	std::vector<float> intensity;

	Point3f lowerBoundary;
	Point3f upperBoundary;
	Point3f centerPoint;
	float width, height, depth;

	void Reset()
	{
		points.clear();
		intensity.clear();
		lowerBoundary = Point3f();
		upperBoundary = Point3f();
		centerPoint = Point3f();
		width = height = depth = 0;
	}
} gPointsCloud;

void OnMouse3d(int event, int x, int y, int flags, void* param)
{
	if (event == CV_EVENT_RBUTTONDOWN) {
		gThetaX = 0;
		gThetaY = 0;
		gScaleFactor = 1;
	}

	if (event == CV_EVENT_LBUTTONDOWN) {
		dxOld = x;
		dyOld = y;
	}

	if (event == CV_EVENT_MOUSEMOVE && (flags & CV_EVENT_LBUTTONDOWN))   //左键按下，鼠标移动时
	{
		dx += x - dxOld;
		dy += y - dyOld;
		gThetaX = dy / 30;
		gThetaY = dx / 30;
	}

	if (event == CV_EVENT_MOUSEWHEEL) {
		int value = getMouseWheelDelta(flags);
		float scaleStep = SCALE_STEP;
		if (value > 0)
			scaleStep = +SCALE_STEP;
		else if (value < 0)
			scaleStep = -SCALE_STEP;
		gScaleFactor *= (1 + scaleStep);
	}

	updateWindow(gWindow3dName);
}

void OnOpengl(void* param)
{
	//glViewport(0, 0, (GLsizei)gPointsCloud.width, (GLsizei)gPointsCloud.height);
	//glMatrixMode(GL_PROJECTION);

	glLoadIdentity();
	glScalef(gScaleFactor, gScaleFactor, gScaleFactor);
	glTranslatef(-gPointsCloud.centerPoint.x, -gPointsCloud.centerPoint.y, -gPointsCloud.centerPoint.z);
	glRotatef(gThetaX, 1, 0, 0);
	glRotatef(gThetaY, 0, 0, 1);

	glBegin(GL_POINTS);
	for (size_t i = 0; i < gPointsCloud.points.size(); i++) {
		glPointSize(gPointsCloud.intensity[i]/100);
		glColor3f(0, 1, 0);
		glVertex3f(gPointsCloud.points[i].x, gPointsCloud.points[i].y, gPointsCloud.points[i].z);
		
	}
	glEnd();
	glFlush();
}

bool LoadData()
{
	FILE *file = fopen("../data/xyzi.txt", "r");
	if (file == NULL) {
		return false;
	}

	char line_buffer[MAX_LINE_BUFFER_SIZE];
	float x = 0, y = 0, z = 0, i = 0;
	gPointsCloud.Reset();

	bool isFirstIn = true;
	while (fgets(line_buffer, MAX_LINE_BUFFER_SIZE, file)) {
		if (sscanf(line_buffer, "%f %f %f %f", &x, &y, &z, &i) == 4) {
			gPointsCloud.points.push_back(Point3f(x, y, z));
			gPointsCloud.intensity.push_back(i);

			if (isFirstIn) {
				gPointsCloud.lowerBoundary.x = gPointsCloud.upperBoundary.x = x;
				gPointsCloud.lowerBoundary.y = gPointsCloud.upperBoundary.y = y;
				gPointsCloud.lowerBoundary.z = gPointsCloud.upperBoundary.z = z;
				isFirstIn = false;
			}
			gPointsCloud.lowerBoundary.x = MIN(gPointsCloud.lowerBoundary.x, x);
			gPointsCloud.lowerBoundary.y = MIN(gPointsCloud.lowerBoundary.y, y);
			gPointsCloud.lowerBoundary.z = MIN(gPointsCloud.lowerBoundary.z, z);

			gPointsCloud.upperBoundary.x = MAX(gPointsCloud.upperBoundary.x, x);
			gPointsCloud.upperBoundary.y = MAX(gPointsCloud.upperBoundary.y, y);
			gPointsCloud.upperBoundary.z = MAX(gPointsCloud.upperBoundary.z, z);
		}
	}
	fclose(file);

	gPointsCloud.width = gPointsCloud.upperBoundary.x - gPointsCloud.lowerBoundary.x;
	gPointsCloud.height = gPointsCloud.upperBoundary.y - gPointsCloud.lowerBoundary.y;
	gPointsCloud.depth = gPointsCloud.upperBoundary.z - gPointsCloud.lowerBoundary.z;

	gPointsCloud.centerPoint.x = (gPointsCloud.upperBoundary.x + gPointsCloud.lowerBoundary.x)/2;
	gPointsCloud.centerPoint.y = (gPointsCloud.upperBoundary.y + gPointsCloud.lowerBoundary.y)/2;
	gPointsCloud.centerPoint.z = (gPointsCloud.upperBoundary.z + gPointsCloud.lowerBoundary.z)/2;

	return true;
}

int main()
{
	namedWindow(gWindow2dName, WINDOW_AUTOSIZE);
	setMouseCallback(gWindow2dName, OnMouse2d);

	if (LoadData()) {
		namedWindow(gWindow3dName, WINDOW_OPENGL);
		resizeWindow(gWindow3dName, 640, 480);
		setOpenGlContext(gWindow3dName);
		setMouseCallback(gWindow3dName, OnMouse3d);
		setOpenGlDrawCallback(gWindow3dName, OnOpengl);
	}

	bool runFlag = true;
	while (runFlag) {
		int key = waitKey(2);
		switch (key) {
		case 'q':
		{
			runFlag = false;
			break;
		}
		default:
			break;
		}
	}

	destroyAllWindows();
	return 0;
}