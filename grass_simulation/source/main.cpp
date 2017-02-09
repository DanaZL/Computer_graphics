#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <cmath>
#include "SOIL.h"

#include "Utility.h"

using namespace std;

const uint GRASS_INSTANCES = 10000; // Количество травинок

GL::Camera camera;               // Мы предоставляем Вам реализацию камеры. В OpenGL камера - это просто 2 матрицы. Модельно-видовая матрица и матрица проекции. // ###
                                 // Задача этого класса только в том чтобы обработать ввод с клавиатуры и правильно сформировать эти матрицы.
                                 // Вы можете просто пользоваться этим классом для расчёта указанных матриц.


GLuint grassPointsCount; // Количество вершин у модели травинки
GLuint grassShader;      // Шейдер, рисующий траву 
GLuint grassVAO;         // VAO для травы (что такое VAO почитайте в доках)
GLuint grassVariance;    // Буфер для смещения координат травинок
GLuint grassInitVariance;    // Буфер для начальных смещений координат травинок
GLuint lastVariance;    //Буфер для предыдущего изменения координаты x
GLuint texture;
GLuint textureGrass;
vector<VM::vec4> grassVarianceData(GRASS_INSTANCES); // Вектор со смещениями для координат травинок
//ориентация травинки, ее масштаб и оттенок
//начальный наклон - координата по x и z
vector<VM::vec4> grassInitVarianceData(GRASS_INSTANCES); // Вектор с начальными смещениями смещениями
vector<float> lastVarianceData(GRASS_INSTANCES); //вектор с последними изменениями координаты 
vector<VM::vec2> grassPositionsData(GRASS_INSTANCES);

GLuint groundShader; // Шейдер для земли
GLuint groundVAO; // VAO для земли

// Размеры экрана
uint screenWidth = 800;
uint screenHeight = 600;

// Это для захвата мышки. Вам это не потребуется (это не значит, что нужно удалять эту строку)
bool captureMouse = true;

// Функция, рисующая землю
void DrawGround() {
    // Используем шейдер для земли
    glUseProgram(groundShader);                                                  CHECK_GL_ERRORS

    // Устанавливаем юниформ для шейдера. В данном случае передадим перспективную матрицу камеры
    // Находим локацию юниформа 'camera' в шейдере
    GLint cameraLocation = glGetUniformLocation(groundShader, "camera");         CHECK_GL_ERRORS
    // Устанавливаем юниформ (загружаем на GPU матрицу проекции?)                                                     // ###
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS

    glBindTexture(GL_TEXTURE_2D, texture);  
    // Подключаем VAO, который содержит буферы, необходимые для отрисовки земли
    glBindVertexArray(groundVAO);                                               CHECK_GL_ERRORS


    // Рисуем землю: 2 треугольника (6 вершин)
    glDrawArrays(GL_TRIANGLES, 0, 6);                                            CHECK_GL_ERRORS

    // Отсоединяем VAO
    glBindTexture(GL_TEXTURE_2D, 0);    
    glBindVertexArray(0); 
                                             CHECK_GL_ERRORS
    // Отключаем шейдер
    glUseProgram(0);                                                             CHECK_GL_ERRORS
}

// Обновление смещения травинок
void UpdateGrassVariance() {
    static vector<float> lastVarianceData_local(GRASS_INSTANCES);

    lastVarianceData = lastVarianceData_local;  
    for (uint i = 0; i < GRASS_INSTANCES; ++i) {
        float amplitude = 0.06;
        float omega = 0.000002;
        float C = 0.8;
        int t = clock();
        grassVarianceData[i].x = amplitude * sin(omega * t + grassPositionsData[i].x / C);
        // grassVarianceData[i].y = grassVarianceData[i].x
        grassVarianceData[i].z = 0.0;
        lastVarianceData_local[i] = grassVarianceData[i].x;
        // cout << lastVarianceData[1]  - grassVarianceData[1].x<< endl;   
    }

    // Привязываем буфер, содержащий смещения
    glBindBuffer(GL_ARRAY_BUFFER, grassVariance);                                CHECK_GL_ERRORS
    // Загружаем данные в видеопамять
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * GRASS_INSTANCES, grassVarianceData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS
    // Отвязываем буфер
    glBindBuffer(GL_ARRAY_BUFFER, 0);                                            CHECK_GL_ERRORS
}


// Рисование травы
void DrawGrass() {
    // Тут то же самое, что и в рисовании земли
    glUseProgram(grassShader);                                                   CHECK_GL_ERRORS
    GLint cameraLocation = glGetUniformLocation(grassShader, "camera");          CHECK_GL_ERRORS
    glUniformMatrix4fv(cameraLocation, 1, GL_TRUE, camera.getMatrix().data().data()); CHECK_GL_ERRORS
      
    glBindVertexArray(grassVAO);     
    glBindTexture(GL_TEXTURE_2D, textureGrass);                                                CHECK_GL_ERRORS
    // Обновляем смещения для травы
    UpdateGrassVariance();
    // Отрисовка травинок в количестве GRASS_INSTANCES
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, grassPointsCount, GRASS_INSTANCES);   CHECK_GL_ERRORS 
    glBindTexture(GL_TEXTURE_2D, 0); 
    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    glUseProgram(0);                                                             CHECK_GL_ERRORS
}

// Эта функция вызывается для обновления экрана
void RenderLayouts() {
    // Включение буфера глубины
    glEnable(GL_DEPTH_TEST);
    // Очистка буфера глубины и цветового буфера
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Рисуем меши
    DrawGround();
    DrawGrass();  
    glutSwapBuffers();
}

// Завершение программы
void FinishProgram() {
    glutDestroyWindow(glutGetWindow());
}

uint anti_aliasing = 0;
// Обработка события нажатия клавиши (специальные клавиши обрабатываются в функции SpecialButtons)
void KeyboardEvents(unsigned char key, int x, int y) {
    if (key == 27) {
        FinishProgram();
    } else if (key == 'w') {
        camera.goForward();
    } else if (key == 's') {
        camera.goBack();
    } else if (key == 'm') {
        captureMouse = !captureMouse;
        if (captureMouse) {
            glutWarpPointer(screenWidth / 2, screenHeight / 2);
            glutSetCursor(GLUT_CURSOR_NONE);
        } else {
            glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);
        }
    } else if (key == 'A') {
        if (anti_aliasing) { 
            cout << "Anti-aliasing: OFF" << endl; 
            glDisable(GL_MULTISAMPLE);
            anti_aliasing = 0;
        } else {
            cout << "Anti-aliasing: ON" << endl;  
            glEnable(GL_MULTISAMPLE);
            glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
            GLint iMultiSample = 0;
            GLint iNumSamples = 0;
            glGetIntegerv(GL_SAMPLE_BUFFERS, &iMultiSample);
            glGetIntegerv(GL_SAMPLES, &iNumSamples);
            anti_aliasing = 1; 
        }
    }
}

// Обработка события нажатия специальных клавиш
void SpecialButtons(int key, int x, int y) {
    if (key == GLUT_KEY_RIGHT) {
        camera.rotateY(0.02);
    } else if (key == GLUT_KEY_LEFT) {
        camera.rotateY(-0.02);
    } else if (key == GLUT_KEY_UP) {
        camera.rotateTop(-0.02);
    } else if (key == GLUT_KEY_DOWN) {
        camera.rotateTop(0.02);
    }
}

void IdleFunc() {
    glutPostRedisplay();
}

// Обработка события движения мыши
void MouseMove(int x, int y) {
    if (captureMouse) {
        int centerX = screenWidth / 2,
            centerY = screenHeight / 2;
        if (x != centerX || y != centerY) {
            camera.rotateY((x - centerX) / 1000.0f);
            camera.rotateTop((y - centerY) / 1000.0f);
            glutWarpPointer(centerX, centerY);
        }
    }
}

// Обработка нажатия кнопки мыши
void MouseClick(int button, int state, int x, int y) {
}

// Событие изменение размера окна
void windowReshapeFunc(GLint newWidth, GLint newHeight) {
    glViewport(0, 0, newWidth, newHeight);
    screenWidth = newWidth;
    screenHeight = newHeight;

    camera.screenRatio = (float)screenWidth / screenHeight;
}

// Инициализация окна
void InitializeGLUT(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE | GLUT_MULTISAMPLE);
    glutInitContextVersion(3, 0);
    glutInitWindowPosition(-1, -1);
    glutInitWindowSize(screenWidth, screenHeight);
    glutCreateWindow("Computer Graphics 3");
    glutWarpPointer(400, 300);
    glutSetCursor(GLUT_CURSOR_NONE);

    glutDisplayFunc(RenderLayouts);
    glutKeyboardFunc(KeyboardEvents);
    glutSpecialFunc(SpecialButtons);
    glutIdleFunc(IdleFunc);
    glutPassiveMotionFunc(MouseMove);
    glutMouseFunc(MouseClick);
    glutReshapeFunc(windowReshapeFunc);
}

// Генерация позиций травинок (эту функцию вам придётся переписать)
vector<VM::vec2> GenerateGrassPositions() {

    vector<VM::vec2> grassPositions(GRASS_INSTANCES);

    uint cnt_grass_in_line = ceil(sqrt(GRASS_INSTANCES)); 
    float grass_dist = 1.0 / (cnt_grass_in_line + 1.0);
    srand(time(0));

    for (uint i = 0; i < GRASS_INSTANCES; ++i) {

        float length = float(rand() + float(RAND_MAX)/4) / (float(RAND_MAX));

        float rand_sign_1 = ((float(rand()) / RAND_MAX) > 0.5) ? 1 : -1; 
        float rand_sign_2 = ((float(rand()) / RAND_MAX) > 0.5) ? 1 : -1; 
        float incline_x = rand_sign_1 * (float(rand()) / RAND_MAX) / 30.0;
        float incline_z = rand_sign_2 * (float(rand()) / RAND_MAX) / 30.0;

        // cout << incline << endl;
        
        if (length > 1) length = 1;

        uint angle = rand() % 180;

        grassInitVarianceData[i] = VM::vec4();
        grassInitVarianceData[i][0] = (angle * M_PI) / 180;
        grassInitVarianceData[i][1] = length;
        
        grassInitVarianceData[i][2] =  incline_x;
        grassInitVarianceData[i][3] = incline_z;
        

        float x = grass_dist * (i / cnt_grass_in_line + 1);
        float y = grass_dist * (i % cnt_grass_in_line + 1);
        
        float noize_x = ((rand() - rand()) / (float(RAND_MAX) + 1)) * grass_dist;
        float noize_y = ((rand() - rand()) / (float(RAND_MAX) + 1)) * grass_dist;

        grassPositions[i] = VM::vec2(x + noize_x, y + noize_y);
        //grassPositions[i] = VM::vec2((i % 4) / 4.0, (i / 4) / 4.0) + VM::vec2(1, 1) / 8;
    }


    // grassPositions[0] = VM::vec2(0.2, 0.2);
    // grassPositions[1] = VM::vec2(0.25, 0.25);

    return grassPositions;
}

// Здесь вам нужно будет генерировать меш
vector<VM::vec4> GenMesh(uint n) {
    return {
        // VM::vec4(0, 0, 0, 1),
        // VM::vec4(1, 0, 0, 1),
        // VM::vec4(0.5, 1, 0, 1)

        VM::vec4(0, 0, 0, 1),
        
        VM::vec4(0.6, 0, 0, 1),
        VM::vec4(0, 0.4, 0, 1),

        VM::vec4(0.6, 0.5, 0, 1),

        VM::vec4(0.3, 0.7, 0, 1),
        VM::vec4(0.75, 0.8, 0, 1),
        VM::vec4(0.9, 1, 0, 1)     
    };
}

// Создание травы
void CreateGrass() {
    srand(time(0));
    uint LOD = 1;
    // Создаём меш
    vector<VM::vec4> grassPoints = GenMesh(LOD);
    // Сохраняем количество вершин в меше травы
    grassPointsCount = grassPoints.size();
    // Создаём позиции для травинок
    vector<VM::vec2> grassPositions = GenerateGrassPositions();
    grassPositionsData = grassPositions;
    // Инициализация смещений для травинок
    for (uint i = 0; i < GRASS_INSTANCES; ++i) {
        grassVarianceData[i] = VM::vec4(0, 0, 0, 0);
    }

    vector<VM::vec2> grassTexturePoints(grassPointsCount);
    for (uint i = 0; i < grassPointsCount; ++i) {
        cout << i<< endl;
        grassTexturePoints[i].x = grassPoints[i].y;
        grassTexturePoints[i].y = grassPoints[i].x; 
    }

    cout << grassTexturePoints[1]<< endl;

    /* Компилируем шейдеры
    Эта функция принимает на вход название шейдера 'shaderName',
    читает файлы shaders/{shaderName}.vert - вершинный шейдер
    и shaders/{shaderName}.frag - фрагментный шейдер,
    компилирует их и линкует.
    */

    grassShader = GL::CompileShaderProgram("grass");

    // Здесь создаём буфер
    GLuint pointsBuffer;
    // Это генерация одного буфера (в pointsBuffer хранится идентификатор буфера)
    glGenBuffers(1, &pointsBuffer);                                              CHECK_GL_ERRORS
    // Привязываем сгенерированный буфер
    glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer);                                 CHECK_GL_ERRORS
    // Заполняем буфер данными из вектора
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * grassPoints.size(), grassPoints.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    // Создание VAO
    // Генерация VAO
    glGenVertexArrays(1, &grassVAO);                                             CHECK_GL_ERRORS
    // Привязка VAO
    glBindVertexArray(grassVAO);                                                 CHECK_GL_ERRORS

    // Получение локации параметра 'point' в шейдере
    GLuint pointsLocation = glGetAttribLocation(grassShader, "point");           CHECK_GL_ERRORS
    // Подключаем массив атрибутов к данной локации
    glEnableVertexAttribArray(pointsLocation);                                   CHECK_GL_ERRORS
    // Устанавливаем параметры для получения данных из массива (по 4 значение типа float на одну вершину)
    glVertexAttribPointer(pointsLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);          CHECK_GL_ERRORS

    // Создаём буфер для позиций травинок
    GLuint positionBuffer;
    glGenBuffers(1, &positionBuffer);                                            CHECK_GL_ERRORS
    // Здесь мы привязываем новый буфер, так что дальше вся работа будет с ним до следующего вызова glBindBuffer
    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * grassPositions.size(), grassPositions.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint positionLocation = glGetAttribLocation(grassShader, "position");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(positionLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
    // Здесь мы указываем, что нужно брать новое значение из этого буфера для каждого инстанса (для каждой травинки)
    glVertexAttribDivisor(positionLocation, 1);                                  CHECK_GL_ERRORS

    // Создаём буфер для смещения травинок
    glGenBuffers(1, &grassVariance);                                            CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, grassVariance);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * GRASS_INSTANCES, grassVarianceData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint varianceLocation = glGetAttribLocation(grassShader, "variance");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(varianceLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(varianceLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
    glVertexAttribDivisor(varianceLocation, 1);                                  CHECK_GL_ERRORS

     // Создаём буфер для начального смещения травинок
    glGenBuffers(1, &grassInitVariance);                                            CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, grassInitVariance);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) *  GRASS_INSTANCES, grassInitVarianceData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint initVarianceLocation = glGetAttribLocation(grassShader, "init_variance");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(initVarianceLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(initVarianceLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
    glVertexAttribDivisor(initVarianceLocation, 1);                                  CHECK_GL_ERRORS


    // Создаём буфер для смещения травинок
    glGenBuffers(1, &lastVariance);                                            CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, lastVariance);                               CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * GRASS_INSTANCES, lastVarianceData.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint lastVarianceLocation = glGetAttribLocation(grassShader, "last_variance");      CHECK_GL_ERRORS
    glEnableVertexAttribArray(lastVarianceLocation);                                 CHECK_GL_ERRORS
    glVertexAttribPointer(lastVarianceLocation, 1, GL_FLOAT, GL_FALSE, 0, 0);        CHECK_GL_ERRORS
    glVertexAttribDivisor(lastVarianceLocation, 1);   

    GLuint textureBuffer;
    glGenBuffers(1, &textureBuffer);                                              CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);                                 CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * grassTexturePoints.size(), grassTexturePoints.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint textloc = glGetAttribLocation(grassShader, "textPoints");              CHECK_GL_ERRORS
    glEnableVertexAttribArray(textloc);                                            CHECK_GL_ERRORS
    glVertexAttribPointer(textloc, 2 , GL_FLOAT, GL_FALSE, 0, 0);                  CHECK_GL_ERRORS

    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    glGenTextures(1, &textureGrass);
    //glActiveTexture(GL_TEXTURE0);                                                 CHECK_GL_ERRORS
    glBindTexture(GL_TEXTURE_2D, textureGrass);  
    // glUniform1i(glGetUniformLocation(groundShader, "ourTexture"), 0);                                    CHECK_GL_ERRORS

    int width, height;
    unsigned char* image = SOIL_load_image("../bin/Texture/grass.jpg", &width, &height, 0, SOIL_LOAD_RGBA); CHECK_GL_ERRORS
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);        CHECK_GL_ERRORS
    glGenerateMipmap(GL_TEXTURE_2D);                                                                    CHECK_GL_ERRORS

    SOIL_free_image_data(image);
    glBindTexture(GL_TEXTURE_2D, 0);  
}


// Создаём камеру (Если шаблонная камера вам не нравится, то можете переделать, но я бы не стал)
void CreateCamera() {
    camera.angle = 45.0f / 180.0f * M_PI;
    camera.direction = VM::vec3(0, 0.3, -1);
    camera.position = VM::vec3(0.5, 0.2, 0);
    camera.screenRatio = (float)screenWidth / screenHeight;
    camera.up = VM::vec3(0, 1, 0);
    camera.zfar = 50.0f;
    camera.znear = 0.05f;
}

// Создаём замлю
void CreateGround() {
    // Земля состоит из двух треугольников
    vector<VM::vec4> meshPoints = {
        VM::vec4(0, 0, 0, 1),
        VM::vec4(1, 0, 0, 1),
        VM::vec4(1, 0, 1, 1),
        VM::vec4(0, 0, 0, 1),
        VM::vec4(1, 0, 1, 1),
        VM::vec4(0, 0, 1, 1),
    };

    vector<VM::vec2> textPoints = {
        VM::vec2(0, 0),
        VM::vec2(1, 0),
        VM::vec2(1, 1),
        VM::vec2(0, 0),
        VM::vec2(1, 1),
        VM::vec2(0, 1),
    };

    groundShader = GL::CompileShaderProgram("ground");

    GLuint pointsBuffer;
    glGenBuffers(1, &pointsBuffer);                                              CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, pointsBuffer);                                 CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec4) * meshPoints.size(), meshPoints.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    glGenVertexArrays(1, &groundVAO);                                            CHECK_GL_ERRORS
    glBindVertexArray(groundVAO);                                                CHECK_GL_ERRORS

    GLuint index = glGetAttribLocation(groundShader, "point");                   CHECK_GL_ERRORS
    glEnableVertexAttribArray(index);                                            CHECK_GL_ERRORS
    glVertexAttribPointer(index, 4, GL_FLOAT, GL_FALSE, 0, 0);                   CHECK_GL_ERRORS


    GLuint textureBuffer;
    glGenBuffers(1, &textureBuffer);                                              CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);                                 CHECK_GL_ERRORS
    glBufferData(GL_ARRAY_BUFFER, sizeof(VM::vec2) * textPoints.size(), textPoints.data(), GL_STATIC_DRAW); CHECK_GL_ERRORS

    GLuint textloc = glGetAttribLocation(groundShader, "textPoints");              CHECK_GL_ERRORS
    glEnableVertexAttribArray(textloc);                                            CHECK_GL_ERRORS
    glVertexAttribPointer(textloc, 2 , GL_FLOAT, GL_FALSE, 0, 0);                  CHECK_GL_ERRORS

    glBindVertexArray(0);                                                        CHECK_GL_ERRORS
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    glGenTextures(1, &texture);
    //glActiveTexture(GL_TEXTURE0);                                                 CHECK_GL_ERRORS
    glBindTexture(GL_TEXTURE_2D, texture);  
    // glUniform1i(glGetUniformLocation(groundShader, "ourTexture"), 0);                                    CHECK_GL_ERRORS

    int width, height;
    unsigned char* image = SOIL_load_image("../bin/Texture/ground.jpg", &width, &height, 0, SOIL_LOAD_RGBA); CHECK_GL_ERRORS
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);        CHECK_GL_ERRORS
    glGenerateMipmap(GL_TEXTURE_2D);                                                                    CHECK_GL_ERRORS

    SOIL_free_image_data(image);
    glBindTexture(GL_TEXTURE_2D, 0);                                            CHECK_GL_ERRORS
}

int main(int argc, char **argv)
{
    putenv("MESA_GL_VERSION_OVERRIDE=3.3COMPAT");
    try {
        cout << "Start" << endl;
        InitializeGLUT(argc, argv);
        cout << "GLUT inited" << endl;
        glewInit();
        cout << "glew inited" << endl;
        CreateCamera();
        cout << "Camera created" << endl;
        CreateGrass();
        cout << "Grass created" << endl;
        CreateGround();
        cout << "Ground created" << endl;
        glutMainLoop();
    } catch (string s) {
        cout << s << endl;
    }
}
