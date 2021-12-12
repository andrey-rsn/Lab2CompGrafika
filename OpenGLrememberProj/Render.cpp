
#include "Render.h"

#include <sstream>
#include <iostream>

#include <windows.h>
#include <GL\GL.h>
#include <GL\GLU.h>

#include "MyOGL.h"

#include "Camera.h"
#include "Light.h"
#include "Primitives.h"

#include "GUItextRectangle.h"
#include<math.h>

double kvYrav(double, double, double, int);
void floorAndTopCreator(double);
void sidesCreator();
void oneSideCreator(double*, double*,double,double,double,double);
double* NCalculator(double [], double [], double []);

bool textureMode = true;
bool lightMode = true;

//класс для настройки камеры
class CustomCamera : public Camera
{
public:
	//дистанция камеры
	double camDist;
	//углы поворота камеры
	double fi1, fi2;

	
	//значния масеры по умолчанию
	CustomCamera()
	{
		camDist = 15;
		fi1 = 1;
		fi2 = 1;
	}

	
	//считает позицию камеры, исходя из углов поворота, вызывается движком
	void SetUpCamera()
	{
		//отвечает за поворот камеры мышкой
		lookPoint.setCoords(0, 0, 0);

		pos.setCoords(camDist*cos(fi2)*cos(fi1),
			camDist*cos(fi2)*sin(fi1),
			camDist*sin(fi2));

		if (cos(fi2) <= 0)
			normal.setCoords(0, 0, -1);
		else
			normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		//функция настройки камеры
		gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
	}



}  camera;   //создаем объект камеры


//Класс для настройки света
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//начальная позиция света
		pos = Vector3(1, 1, 3);
	}

	
	//рисует сферу и линии под источником света, вызывается движком
	void  DrawLightGhismo()
	{
		glDisable(GL_LIGHTING);

		
		glColor3d(0.9, 0.8, 0);
		Sphere s;
		s.pos = pos;
		s.scale = s.scale*0.08;
		s.Show();
		
		if (OpenGL::isKeyPressed('G'))
		{
			glColor3d(0, 0, 0);
			//линия от источника света до окружности
			glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//рисуем окруность
			Circle c;
			c.pos.setCoords(pos.X(), pos.Y(), 0);
			c.scale = c.scale*1.5;
			c.Show();
		}

	}

	void SetUpLight()
	{
		GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
		GLfloat spec[] = { .7, .7, .7, 0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}


} light;  //создаем источник света




//старые координаты мыши
int mouseX = 0, mouseY = 0;

void mouseEvent(OpenGL *ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//меняем углы камеры при нажатой левой кнопке мыши
	if (OpenGL::isKeyPressed(VK_RBUTTON))
	{
		camera.fi1 += 0.01*dx;
		camera.fi2 += -0.01*dy;
	}

	
	//двигаем свет по плоскости, в точку где мышь
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y);

		double z = light.pos.Z();

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0)
			k = 0;
		else
			k = (z - r.origin.Z()) / r.direction.Z();

		x = k*r.direction.X() + r.origin.X();
		y = k*r.direction.Y() + r.origin.Y();

		light.pos = Vector3(x, y, z);
	}

	if (OpenGL::isKeyPressed('G') && OpenGL::isKeyPressed(VK_LBUTTON))
	{
		light.pos = light.pos + Vector3(0, 0, 0.02*dy);
	}

	
}

void mouseWheelEvent(OpenGL *ogl, int delta)
{

	if (delta < 0 && camera.camDist <= 1)
		return;
	if (delta > 0 && camera.camDist >= 100)
		return;

	camera.camDist += 0.01*delta;

}

void keyDownEvent(OpenGL *ogl, int key)
{
	if (key == 'L')
	{
		lightMode = !lightMode;
	}

	if (key == 'T')
	{
		textureMode = !textureMode;
	}

	if (key == 'R')
	{
		camera.fi1 = 1;
		camera.fi2 = 1;
		camera.camDist = 15;

		light.pos = Vector3(1, 1, 3);
	}

	if (key == 'F')
	{
		light.pos = camera.pos;
	}
}

void keyUpEvent(OpenGL *ogl, int key)
{
	
}



GLuint texId;

//выполняется перед первым рендером
void initRender(OpenGL *ogl)
{
	//настройка текстур

	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//включаем текстуры
	glEnable(GL_TEXTURE_2D);
	

	//массив трехбайтных элементов  (R G B)
	RGBTRIPLE *texarray;

	//массив символов, (высота*ширина*4      4, потомучто   выше, мы указали использовать по 4 байта на пиксель текстуры - R G B A)
	char *texCharArray;
	int texW, texH;
	OpenGL::LoadBMP("texture.bmp", &texW, &texH, &texarray);
	OpenGL::RGBtoChar(texarray, texW, texH, &texCharArray);

	
	
	//генерируем ИД для текстуры
	glGenTextures(1, &texId);
	//биндим айдишник, все что будет происходить с текстурой, будте происходить по этому ИД
	glBindTexture(GL_TEXTURE_2D, texId);

	//загружаем текстуру в видеопямять, в оперативке нам больше  она не нужна
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, texCharArray);

	//отчистка памяти
	free(texCharArray);
	free(texarray);

	//наводим шмон
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


	//камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;
	ogl->mainLight = &light;

	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// устранение ступенчатости для линий
	glEnable(GL_LINE_SMOOTH); 


	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	camera.fi1 = -1.3;
	camera.fi2 = 0.8;
}





void Render(OpenGL *ogl)
{



	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);
	if (textureMode)
		glEnable(GL_TEXTURE_2D);

	if (lightMode)
		glEnable(GL_LIGHTING);


	//альфаналожение
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	//настройка материала
	GLfloat amb[] = { 0.2, 0.2, 0.1, 1. };
	GLfloat dif[] = { 0.4, 0.65, 0.5, 1. };
	GLfloat spec[] = { 0.9, 0.8, 0.3, 1. };
	GLfloat sh = 0.1f * 256;


	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec); \
		//размер блика
		glMaterialf(GL_FRONT, GL_SHININESS, sh);

	//чтоб было красиво, без квадратиков (сглаживание освещения)
	glShadeModel(GL_SMOOTH);
	//===================================
	//Прогать тут  


	//Начало рисования квадратика станкина

	/*double A[2] = {-4, -4};
	double B[2] = { 4, -4 };
	double C[2] = { 4, 4 };
	double D[2] = { -4, 4 };

	glBindTexture(GL_TEXTURE_2D, texId);

	glColor3d(0.6, 0.6, 0.6);
	glBegin(GL_QUADS);

	glNormal3d(0, 0, 1);
	glTexCoord2d(0, 0);
	glVertex2dv(A);
	glTexCoord2d(1, 0);
	glVertex2dv(B);
	glTexCoord2d(1, 1);
	glVertex2dv(C);
	glTexCoord2d(0, 1);
	glVertex2dv(D);

	glEnd();*/
	for (int i = 0; i < 2; i++)
	{
		floorAndTopCreator(10 * i);
	}

	sidesCreator();
	//конец рисования квадратика станкина


   //Сообщение вверху экрана

	
	glMatrixMode(GL_PROJECTION);	//Делаем активной матрицу проекций. 
	                                //(всек матричные операции, будут ее видоизменять.)
	glPushMatrix();   //сохраняем текущую матрицу проецирования (которая описывает перспективную проекцию) в стек 				    
	glLoadIdentity();	  //Загружаем единичную матрицу
	glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);	 //врубаем режим ортогональной проекции

	glMatrixMode(GL_MODELVIEW);		//переключаемся на модел-вью матрицу
	glPushMatrix();			  //сохраняем текущую матрицу в стек (положение камеры, фактически)
	glLoadIdentity();		  //сбрасываем ее в дефолт

	glDisable(GL_LIGHTING);



	GuiTextRectangle rec;		   //классик моего авторства для удобной работы с рендером текста.
	rec.setSize(300, 150);
	rec.setPosition(10, ogl->getHeight() - 150 - 10);


	std::stringstream ss;
	ss << "T - вкл/выкл текстур" << std::endl;
	ss << "L - вкл/выкл освещение" << std::endl;
	ss << "F - Свет из камеры" << std::endl;
	ss << "G - двигать свет по горизонтали" << std::endl;
	ss << "G+ЛКМ двигать свет по вертекали" << std::endl;
	ss << "Коорд. света: (" << light.pos.X() << ", " << light.pos.Y() << ", " << light.pos.Z() << ")" << std::endl;
	ss << "Коорд. камеры: (" << camera.pos.X() << ", " << camera.pos.Y() << ", " << camera.pos.Z() << ")" << std::endl;
	ss << "Параметры камеры: R="  << camera.camDist << ", fi1=" << camera.fi1 << ", fi2=" << camera.fi2 << std::endl;
	
	rec.setText(ss.str().c_str());
	rec.Draw();

	glMatrixMode(GL_PROJECTION);	  //восстанавливаем матрицы проекции и модел-вью обратьно из стека.
	glPopMatrix();


	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

}

double kvYrav(double a, double b, double c, int fl)
{
	double  d, x1, x2; // ќбъ¤вл¤ем переменные с плавающей точкой.


	d = b * b - 4 * a * c; // –ассчитываем дискриминант


	if (d > 0) // ”словие при дискриминанте больше нул¤
	{

		x1 = floor(((((-b) + sqrt(d)) / (2 * a) * 1000))) / 1000;
		x2 = floor(((((-b) - sqrt(d)) / (2 * a) * 1000))) / 1000;
		//x2 = ((-b) - sqrt(d) )/ (2 * a);
		if (fl == 0)
			return x2;
		else
			return x1;
		//cout << "x1 = " << x1 << "\n";
		//cout << "x2 = " << x2 << "\n";
	}
	if (d == 0) // ”словие дл¤ дискриминанта равного нулю
	{
		x1 = -(b / (2 * a));
		//cout << "x1 = x2 = " << x1 << "\n";
		return x1;
	}
	if (d < 0) // ”словие при дискриминанте меньше нул¤
		return NULL;
}

void floorAndTopCreator(double z)
{
	double p1[] = { -1,-2,z };
	double p2[] = { 2.5,-4,z };
	double p3[] = { 2,-2,z };
	double p4[] = { 0,0,z };
	double p5[] = { 1.5,0,z };
	double p6[] = { 4,1.5,z };
	double p7[] = { 0.5,0.5,z };
	double p8[] = { -3.5,1,z };
	double p9[] = { -2.5,3.5,z };

	if(z==0)
	glNormal3d(0, 0, -1);
	if(z==10)
    glNormal3d(0, 0, 1);


	
	glBegin(GL_TRIANGLE_STRIP);

	glColor4f(0.1, 0.7, 0.5, 1);
	//glColor3d(0.5, 0.5, 1);
	glVertex3dv(p1);
	//glColor3d(1, 0.5, 1);
	glVertex3dv(p2);
	//glColor3d(0.5, 1, 1);
	glVertex3dv(p3);

	//glColor3d(0.5, 0.5, 1);
	glVertex3dv(p1);
	//glColor3d(0.5, 1, 1);
	glVertex3dv(p3);
	//glColor3d(0.5, 1, 1);
	glVertex3dv(p4);

	//glColor3d(0.5, 1, 1);
	glVertex3dv(p4);
	//glColor3d(0.5, 1, 1);
	glVertex3dv(p3);
	//glColor3d(0.5, 1, 1);
	glVertex3dv(p5);

	//glColor3d(0.5, 1, 1);
	glVertex3dv(p5);
	//glColor3d(0.5, 1, 1);
	glVertex3dv(p6);
	//glColor3d(0.5, 1, 1);
	glVertex3dv(p7);

	//glColor3d(0.5, 1, 1);
	glVertex3dv(p5);
	//glColor3d(0.5, 1, 1);
	glVertex3dv(p7);
	//glColor3d(0.5, 1, 1);
	glVertex3dv(p4);

	//glColor3d(0.5, 1, 1);
	glVertex3dv(p4);
	//glColor3d(0.5, 1, 1);
	glVertex3dv(p7);
	//glColor3d(0.5, 1, 1);
	glVertex3dv(p8);

	//glColor3d(0.5, 1, 1);
	glVertex3dv(p8);
	//glColor3d(0.5, 1, 1);
	glVertex3dv(p7);
	//glColor3d(0.5, 1, 1);
	glVertex3dv(p9);

	//glColor3d(0.5, 1, 1);
	//glVertex3dv(p8);
	//glColor3d(0.5, 1, 1);
	//glVertex3dv(p7);
	//glColor3d(0.5, 1, 1);
	//glVertex3dv(p9);

	//glColor3d(0.5, 0.5, 1);
	//glVertex3dv(p1);
	//glColor3d(1, 0.5, 1);
	//glVertex3dv(p2);
	//glColor3d(0.5, 1, 1);
	//glVertex3dv(p3);

	glEnd();
	double cx = (p8[0] + p9[0]) / 2;
	double cy = (p8[1] + p9[1]) / 2;

	double R = floor((sqrt(pow((p9[0] - p8[0]), 2) + pow((p9[1] - p8[1]), 2)) / 2) * 100000) / 100000;
	float twoPI = 2 * PI;

	glBegin(GL_TRIANGLE_FAN);

	for (float i = PI + (21.8 * PI / 180); i <= twoPI+ (22 * PI / 180); i += 0.001)
		glVertex3d((sin(i) * R)+ cx, (cos(i)* R)+cy,z);

	glEnd();

	//окружность
	/*
	
	
	double x = p8[0];
	double y = p8[1];
	double h = 0.0025;
	double n_x;
	double n_y;



	int fl = 0;

	glBegin(GL_TRIANGLE_FAN);

	while (x <= (-2.5))
	{

		if (fl == 1)
		{
			n_y = y - h;
		}
		else
		{
			n_y = y + h;

		}
		if (x > (-3.11) && fl == 0)
		{
			fl = 1;
			n_y = y - h;
			y = n_y;
		}



		if (y == p8[1])
			n_x = kvYrav(1, -2 * cx, floor((pow(cx, 2) + pow(n_y - cy, 2) - pow(R, 2)) * 100) / 100, fl);
		else
			n_x = kvYrav(1, -2 * cx, floor((pow(cx, 2) + pow(y - cy, 2) - pow(R, 2)) * 100) / 100, fl);
		if (n_x > (-2.5))
		{
			n_x = -2.5;
			n_y = 3.5;
		}
		//if (n_x == 0)
		//{
		//	n_x = x + h;
		//	n_y = p9[1];
		//}
		double tP[] = { n_x,n_y,z };
		glColor3d(0.5, 1, 1);
		glVertex3dv(tP);
		double oldtP[] = { x,y,z };
		glColor3d(0.5, 1, 1);
		glVertex3dv(oldtP);
		glColor3d(0.5, 1, 1);
		glVertex3dv(p7);
		//if (x == p9[0])
			//break;
		if (n_x < (-2.5))
		{
			x = n_x;
			y = n_y;
		}
		else
			break;
		//if (y > 3.48)
		//{
		//	int a = 0;
		//}
		//if (y >= 3.5)
			//x = p9[0];


	}


	glEnd();*/
	
}

void sidesCreator()
{
	double p1[] = { -1,-2,0 };
	double p2[] = { 2.5,-4,0 };
	double p3[] = { 2,-2,0 };
	double p4[] = { 0,0,0 };
	double p5[] = { 1.5,0,0 };
	double p6[] = { 4,1.5,0 };
	double p7[] = { 0.5,0.5,0 };
	double p8[] = { -3.5,1,0 };
	double p9[] = { -2.5,3.5,0 };

	//грань полукруга
	/*double R = floor((sqrt(pow((p9[0] - p8[0]), 2) + pow((p9[1] - p8[1]), 2)) / 2) * 1000) / 1000;
	double x = p8[0];
	double y = p8[1];
	double h = 0.0025;
	double n_x;
	double n_y;

	double cx = (p8[0] + p9[0]) / 2;
	double cy = (p8[1] + p9[1]) / 2;
	int fl = 0;
	glBegin(GL_TRIANGLE_STRIP);

	while (x <= (-2.5))
	{
		double oldtP[] = { x,y,0 };
		glColor3d(1, 1, 0);
		glVertex3dv(oldtP);
		oldtP[2] = 10;
		glColor3d(1, 1, 0);
		glVertex3dv(oldtP);
		if (fl == 1)
		{
			n_y = y - h;
		}
		else
		{
			n_y = y + h;

		}
		if (x > (-3.11) && fl == 0)
		{
			fl = 1;
			n_y = y - h;
			y = n_y;
		}
		//n_y = y + h;
		if (y == p8[1])
			n_x = kvYrav(1, -2 * cx, (pow(cx, 2) + pow(n_y - cy, 2) - pow(R, 2)), fl);
		else
			n_x = kvYrav(1, -2 * cx, (pow(cx, 2) + pow(y - cy, 2) - pow(R, 2)), fl);
		//n_x = (floor((pow(R, 2) - pow(y, 2))*100)/100)+cx;
		double tP[] = { n_x,n_y,0 };
		glColor3d(1, 1, 0);
		glVertex3dv(tP);

		glColor3d(1, 1, 0);
		glVertex3dv(tP);

		glColor3d(1, 1, 0);
		glVertex3dv(oldtP);

		tP[2] = 10;

		glColor3d(1, 1, 0);
		glVertex3dv(tP);

		x = n_x;
		y = n_y;

	}

	//
	//боковые стороны

	glEnd();*/

	double cx = (p8[0] + p9[0]) / 2;
	double cy = (p8[1] + p9[1]) / 2;

	double R = floor((sqrt(pow((p9[0] - p8[0]), 2) + pow((p9[1] - p8[1]), 2)) / 2) * 100000) / 100000;
	float twoPI = 2 * PI;

	
	glBegin(GL_TRIANGLE_STRIP);
	//double a[] = { -3.5,1,0 };
	//double b[] = { -3.5,1,10 };
	//double c[] = { (-2.5-3.5)/2,(1+3.5 )/2,0};
	//double* Norm = NCalculator(a, b, c);
	//glNormal3d(Norm[0], Norm[1], Norm[2]);
	
	for (float i = PI + (21.8 * PI / 180); i <= twoPI + (22 * PI / 180); i += 0.001)
	{ 
		double a[] = { floor(((sin(i) * R) + cx)*100000)/100000, floor(((cos(i) * R) + cy)*100000)/100000, 0 };
		double c[] = { floor(((sin(i+0.001) * R) + cx) * 100000) / 100000, floor(((cos(i+0.001) * R) + cy) * 100000) / 100000, 0 };
		double b[] = { floor(((sin(i) * R) + cx) * 100000) / 100000, floor(((cos(i) * R) + cy) * 100000) / 100000, 10 };
		double* Norm = NCalculator(a,b,c);
		glNormal3d(Norm[0], Norm[1], Norm[2]);

		glVertex3d((sin(i) * R) + cx, (cos(i) * R) + cy, 0);
	    glVertex3d((sin(i) * R) + cx, (cos(i) * R) + cy, 10);
		
	}

	glEnd();


	oneSideCreator(p1, p2,1,0.5,0.5,1);
	oneSideCreator(p2, p5,0.2,0.4,0.6,1);
	oneSideCreator(p5, p6,0,0.9,0.3,1);
	
	oneSideCreator(p8, p4, 0.1, 0.6, 0.7, 1);
	oneSideCreator(p4, p1, 0.2, 0.3, 0.3, 1);
	//oneSideCreator(p9, p8, 0.7, 0.1, 0.3, 1);
	oneSideCreator(p6, p7, 0.2, 0.1, 0.3, 0.75);
	oneSideCreator(p7, p9, 0.35, 0.1, 0.8, 0.5);
}

void oneSideCreator(double p2[], double p1[],double r,double g,double b,double alpha)
{
	double midP[] = { ((p1[0] + p2[0]) / 2), ((p1[1] + p2[1]) / 2), 0 };
	
	double oldP[] = { p1[0],p1[1],0 };

	double oldPU[] = { p1[0],p1[1],10 };

	double A[] = { p2[0],p2[1],0 };
	double C[] = { p2[0],p2[1],10 };

	double* Norm=NCalculator(A, midP, C);

	glNormal3d(Norm[0],Norm[1],Norm[2]);
	glBegin(GL_TRIANGLE_STRIP);
	
	glColor4f(r, g, b, alpha);
	//glColor3d(0, 1, 0);
	glVertex3dv(oldP);

	oldP[2] = 10;

	//glColor3d(1, 0, 0);
	glVertex3dv(oldP);

	//glColor3d(0, 1, 0);
	glVertex3dv(midP);

	//glColor3d(0, 1, 0);
	glVertex3dv(midP);

	//glColor3d(1, 0, 0);
	glVertex3dv(oldP);

	p2[2] = 10;
	//glColor3d(1, 0, 0);
	glVertex3dv(p2);

	//glColor3d(0, 1, 0);
	glVertex3dv(midP);

	//glColor3d(1, 0, 0);
	glVertex3dv(p2);

	p2[2] = 0;
	//glColor3d(0, 1, 0);
	glVertex3dv(p2);
	glEnd();
}

double* NCalculator(double A[], double B[], double C[])
{
	double a[] = { B[0] - A[0],B[1] - A[1],B[2] - A[2] };
	double b[] = { C[0] - A[0],C[1] - A[1],C[2] - A[2] };
	double N[] = { (a[1]*b[2]-b[1]*a[2]),(-a[0]*b[2]+b[0]*a[2]),(a[0]*b[1]-b[0]*a[1]) };
	double modN = sqrt((pow(N[0], 2) + pow(N[1], 2) + pow(N[2], 2)));

	for (int i = 0; i < 3; i++)
	{
		N[i] = (N[i] / modN);
	}
	
	return N;

}