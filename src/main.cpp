#include "opencv2/opencv.hpp"
#include <Windows.h>
#include <gl/GL.h>
#include <vector>

#define PI						3.1415926535
#define WIDTH					800
#define HEIGHT					800
#define SCALE_STEP_2D			0.1
#define SCALE_STEP_3D			20
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
			scaleStep = SCALE_STEP_2D;
		else if (value < 0)
			scaleStep = -SCALE_STEP_2D;
		gScale2d *= (1 + scaleStep);
		gRoiRect2d.x = x + (gRoiRect2d.x - x)*(1 + scaleStep);
		gRoiRect2d.y = y + (gRoiRect2d.y - y)*(1 + scaleStep);
	}

	Update2d();
}

/* 3d visualization */
float gViewYaw = 0.0;
float gViewPitch = 0.0;
float gViewTransX = 0.0;
float gViewTransY = 0.0;
float gViewDistance = 1000.0;

float gLastX = 0.0;
float gLastY = 0.0;

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
		gLastX = x;
		gLastY = y;
	}

	if (event == CV_EVENT_MOUSEMOVE && (flags & CV_EVENT_RBUTTONDOWN))   //右键按下，鼠标移动时
	{
		gViewTransX += (x - gLastX) * 1.0;
		gViewTransY += (y - gLastY)	* 1.0;
		gLastX = x;
		gLastY = y;
	}

	if (event == CV_EVENT_LBUTTONDOWN) {
		gLastX = x;
		gLastY = y;
	}

	if (event == CV_EVENT_MOUSEMOVE && (flags & CV_EVENT_LBUTTONDOWN))   //左键按下，鼠标移动时
	{
		gViewYaw -= (x - gLastX) * 1.0;
		if (gViewYaw < 0.0) {
			gViewYaw += 360.0;
		} else if (gViewYaw > 360.0) {
			gViewYaw -= 360.0;
		}

		gViewPitch -= (y - gLastY) * 1.0;
		if (gViewPitch < 0.0) {
			gViewPitch += 360.0;
		} else if (gViewPitch > 360.0) {
			gViewPitch -= 360.0;
		}

		gLastX = x;
		gLastY = y;
	}

	if (event == CV_EVENT_MOUSEWHEEL) {
		int value = getMouseWheelDelta(flags);
		if (value > 0) {
			gViewDistance += SCALE_STEP_3D;
		} else if (value < 0) {
			gViewDistance -= SCALE_STEP_3D;
		}

		if (gViewDistance < 1.0) {
			gViewDistance = 1.0;
		} else if (gViewDistance > 2000) {
			gViewDistance = 2000;
		}
	}

	updateWindow(gWindow3dName);
}

void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan(fovy * PI / 360.0);
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;

	glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

void OnOpengl(void* param)
{
	glViewport(0, 0, WIDTH, HEIGHT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, (double)WIDTH / HEIGHT, 1, 5000);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(-gPointsCloud.centerPoint.x, -gPointsCloud.centerPoint.y, -gPointsCloud.centerPoint.z);
	glTranslatef(gViewTransX, -gViewTransY, -gViewDistance);
	glRotatef(-gViewPitch, 1, 0, 0);
	glRotatef(-gViewYaw, 0, 1, 0);

	for (size_t i = 0; i < gPointsCloud.points.size(); i++) {
		glPointSize(gPointsCloud.intensity[i]/100);
		glBegin(GL_POINTS);
		glColor3f(0, 1, 0);
		glVertex3f(gPointsCloud.points[i].x, gPointsCloud.points[i].y, gPointsCloud.points[i].z);
		glEnd();
	}
	
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
		resizeWindow(gWindow3dName, WIDTH, HEIGHT);
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