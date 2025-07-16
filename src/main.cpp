#include "config.h"
typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::milliseconds ms;
typedef std::chrono::duration<float> fsec;

bool isWindowIconified = false; // For minimization
bool hasWindowFocus = true;
float acceleration=10.0f;
int touchingGround=0;
float deltaTime=0.0f;
float gravity=9.8f;


unsigned int make_module(const std::string& filepath, unsigned int module_type);
unsigned int make_shader(const std::string& vertex_filepath, const std::string& fragment_filepath);
GLFWwindow* startGLFW(int width, int height);
void processInput(GLFWwindow *window, glm::vec3& velocity);
std::vector<float> generateCircleVertices(float radius, int segments);

void window_iconify_callback(GLFWwindow* window, int iconified) {
    if (iconified) {
        isWindowIconified = true;
    } else {
        isWindowIconified = false;
    }
}

void window_focus_callback(GLFWwindow* window, int focused) {
    if (focused) {
        hasWindowFocus = true;
    } else {
        hasWindowFocus = false;
    }
}

int main(){
    std::string shaderFilepath="./shaders/";

    GLFWwindow* window;
    int windowWidth=1024;
    int windowHeight=640;

    window=startGLFW(windowWidth,windowHeight);

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    });
    glfwSetWindowIconifyCallback(window, window_iconify_callback);
    glfwSetWindowFocusCallback(window, window_focus_callback);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        glfwTerminate();
        return -1;
    }
    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.0f, 0.0f, 0.f, 1.0f);
    unsigned int shader = make_shader(shaderFilepath+"circle_vertex.txt",shaderFilepath+"circle_fragment.txt");
    GLint projectionLoc = glGetUniformLocation(shader,"projection");
    if (projectionLoc == -1) {
        std::cerr << "Warning: 'projection' uniform not found in shader!" << std::endl;
    }
    GLint modelLoc = glGetUniformLocation(shader, "model");
    if (modelLoc == -1) {
        std::cerr << "Warning: 'model' uniform not found in shader!" << std::endl;
    }
    float radius = 1.0f;
    int segments = 1000;
    std::vector<float> circleVertices = generateCircleVertices(radius, segments);
    int numVertices = circleVertices.size() / 3;

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glfwGetWindowSize(window,&windowWidth,&windowHeight);
    int currentWidth, currentHeight;
    glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
        
    if (currentHeight == 0) currentHeight = 1;

    float aspectRatio = static_cast<float>(currentWidth) / static_cast<float>(currentHeight);
    float orthoHeight = 5.0f;
    float orthoWidth = orthoHeight * aspectRatio;
    glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    auto t0 = Time::now();
    auto t1 = t0;
    while(!glfwWindowShouldClose(window)){
        glfwGetWindowSize(window,&windowWidth,&windowHeight);
        int currentWidth, currentHeight;
        glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
        
        if (currentHeight == 0) currentHeight = 1;

        float aspectRatio = static_cast<float>(currentWidth) / static_cast<float>(currentHeight);
        float oldOrthoWidth = orthoWidth;
        float oldOrthoHeight = orthoHeight;
        float frictionStrength=1.0f;
        
        orthoWidth = orthoHeight * aspectRatio;
        if (oldOrthoWidth != orthoWidth || oldOrthoHeight != orthoHeight) {
            if (glm::length(velocity) < 0.05f) {
                velocity = glm::vec3(0.0f, 0.0f, 0.0f);
            }
        }

        
        t1 = Time::now();
        fsec floatSecs = t1 - t0;
        t0 = t1;
        deltaTime = floatSecs.count();

        
        const float MAX_DELTATIME = 0.1f;
        if (deltaTime > MAX_DELTATIME) {
            deltaTime = MAX_DELTATIME;
        }

        
        if (hasWindowFocus && !isWindowIconified) {
        
            velocity.y -= gravity * deltaTime;
            position += velocity * deltaTime;
            if (glm::abs(velocity.x) > 0.001f) {
                float frictionDirection = -glm::sign(velocity.x); 
                float frictionMagnitude = frictionStrength * deltaTime;

                
                if (glm::abs(velocity.x) < frictionMagnitude) {
                    velocity.x = 0.0f;
                } else {
                    velocity.x += frictionDirection * frictionMagnitude;
                }
            }
        } else {
            if (isWindowIconified) {
                velocity = glm::vec3(0.0f);
            }
        }

        
        if (position.x + radius > orthoWidth) {
            position.x = orthoWidth - radius;
            velocity.x *= -0.95f;
        } else if (position.x - radius < -orthoWidth) {
            position.x = -orthoWidth + radius;
            velocity.x *= -0.95f;
        }

        if (position.y + radius > orthoHeight) {
            position.y = orthoHeight - radius;
            velocity.y *= -0.95f;
        } else if (position.y - radius < -orthoHeight) {
            position.y = -orthoHeight + radius;
            velocity.y *= -0.95f;
        }
        processInput(window,velocity);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shader);

        glm::mat4 projection = glm::ortho(-orthoWidth, orthoWidth, -orthoHeight, orthoHeight, -1.0f, 1.0f);
        if (projectionLoc != -1) {
            glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        }

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        if (modelLoc != -1) {
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        }
        
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_FAN,0,numVertices);
        glBindVertexArray(0);

        glfwPollEvents();
        glfwSwapBuffers(window);
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shader);
    glfwTerminate();

    return 0;
}

GLFWwindow* startGLFW(int width, int height){
    if(!glfwInit()){
        std::cout<<"GLFW couldn't start\n";
        return nullptr;
    }

    GLFWwindow* window=glfwCreateWindow(width,height,"My Window", NULL, NULL);
    return window;
}

void processInput(GLFWwindow *window, glm::vec3& velocity){
    if(glfwGetKey(window, GLFW_KEY_ESCAPE)==GLFW_PRESS){
        glfwSetWindowShouldClose(window,true);
    }
    if(glfwGetKey(window, GLFW_KEY_LEFT)==GLFW_PRESS||glfwGetKey(window,GLFW_KEY_A)==GLFW_PRESS){
        velocity.x+=2.0f;
    }
    else if(glfwGetKey(window, GLFW_KEY_RIGHT)==GLFW_PRESS||glfwGetKey(window,GLFW_KEY_D)==GLFW_PRESS){
        velocity.x-=2.0f;
    }
    if(glfwGetKey(window, GLFW_KEY_UP)==GLFW_PRESS||glfwGetKey(window,GLFW_KEY_W)==GLFW_PRESS){
        velocity.y+=2.0f;
    }
    else if(glfwGetKey(window, GLFW_KEY_DOWN)==GLFW_PRESS||glfwGetKey(window,GLFW_KEY_S)==GLFW_PRESS){
        velocity.y-=2.0f;
    }

}
std::vector<float> generateCircleVertices(float radius, int segments) {
    std::vector<float> vertices;

    vertices.push_back(0.0f); // X
    vertices.push_back(0.0f); // Y
    vertices.push_back(0.0f);

    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * static_cast<float>(i) / static_cast<float>(segments);
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(0.0f);
    }
    return vertices;
}

unsigned int make_shader(const std::string& vertex_filepath, const std::string& fragment_filepath){
    std::vector<unsigned int> modules;
    modules.push_back(make_module(vertex_filepath,GL_VERTEX_SHADER));
    modules.push_back(make_module(fragment_filepath,GL_FRAGMENT_SHADER));

    unsigned int shader = glCreateProgram();
    for (unsigned int shaderModule:modules){
        glAttachShader(shader,shaderModule);

    }
    glLinkProgram(shader);

    int success;
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if(!success){
        char errorLog[1024];
        glGetProgramInfoLog(shader, 1024, NULL, errorLog);
        std::cout<<"Shader link error:\n"<<errorLog<<std::endl;
    }
    return shader;
}

unsigned int make_module(const std::string& filepath, unsigned int module_type){
    std::ifstream file;
    std::stringstream bufferedLines;
    std::string line;

    file.open(filepath);
    while(std::getline(file,line)){
        bufferedLines<<line<<"\n";
    }
    std::string shaderSource = bufferedLines.str();
    const char* shaderSrc = shaderSource.c_str();
    bufferedLines.str("");
    file.close();
    
    unsigned int shaderModule = glCreateShader(module_type);
    glShaderSource(shaderModule, 1, &shaderSrc, NULL);
    glCompileShader(shaderModule);
    
    int success;
    glGetShaderiv(shaderModule,GL_COMPILE_STATUS,&success);
    if(!success){
        char errorLog[1024];
        glGetShaderInfoLog(shaderModule,1024,NULL,errorLog);
        std::cout<<"Shader Module compile error:\n"<<errorLog<<std::endl;
    }
    return shaderModule;
}