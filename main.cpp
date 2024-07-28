#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <random>
#include <GL/glew.h>   // The GL Header File
#include <GL/gl.h>   // The GL Header File
#include <GLFW/glfw3.h> // The GLFW header
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glut.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

using namespace std;

GLuint gProgram[3];

vector<vector<glm::vec3>> initialColors;
vector<std::pair<int,int>> anyAutoMatches;
GLint gIntensityLoc;
float gIntensity = 1000;
int gWidth = 600, gHeight = 640;
int row_count = 0;
int column_count = 0;
double xpos, ypos;
int totalCountGonnaExplode=0;
int tempCount=0;
int moves;
int totalPoints;
// states

enum state_t {BUTTON_CLICKED, ANY_MATCH, STEADY_STATE, ANIMATION}; 
// array of colors

state_t state;

// bool ButtonClicked=false;
// bool autoMatchBoom=false;
// bool steadyState=true;
// bool animationState=false;
static float angle = 0;
static float slidingRate=0;
vector<int> columnCountExploded;


vector<vector<int>> temporaryLocationChange;


struct SlidingValues{
    int columnIndex;
    int count;
    int rowMinIndex;
    int rowMaxIndex;
};
vector<SlidingValues> sliding_objects;


vector<vector<int>> mapExplosionObj;

float explosionRate=1.0f;
float minX = 1e6;
float maxX = -1e6;
float minY = 1e6;
float maxY = -1e6;
float avgX = 0.0f;
float avgY = 0.0f;
struct Vertex
{
    Vertex(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Texture
{
    Texture(GLfloat inU, GLfloat inV) : u(inU), v(inV) { }
    GLfloat u, v;
};

struct Normal
{
    Normal(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Face
{
	Face(int v[], int t[], int n[]) {
		vIndex[0] = v[0];
		vIndex[1] = v[1];
		vIndex[2] = v[2];
		tIndex[0] = t[0];
		tIndex[1] = t[1];
		tIndex[2] = t[2];
		nIndex[0] = n[0];
		nIndex[1] = n[1];
		nIndex[2] = n[2];
	}
    GLuint vIndex[3], tIndex[3], nIndex[3];
};

vector<Vertex> gVertices;
vector<Texture> gTextures;
vector<Normal> gNormals;
vector<Face> gFaces;

// Color vector
vector<glm::vec3> randomColors={glm::vec3(1,0,0),glm::vec3(0,1,0),glm::vec3(0,0,1),glm::vec3(1,1,0),glm::vec3(0,1,1),glm::vec3(1,0,1)};;



GLuint gVertexAttribBuffer, gTextVBO, gIndexBuffer;
GLint gInVertexLoc, gInNormalLoc;
int gVertexDataSizeInBytes, gNormalDataSizeInBytes;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    GLuint TextureID;   // ID handle of the glyph texture
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
    GLuint Advance;    // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;


bool ParseObj(const string& fileName)
{
    fstream myfile;

    // Open the input 
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            stringstream str(curLine);
            GLfloat c1, c2, c3;
            GLuint index[9];
            string tmp;

            if (curLine.length() >= 2)
            {
                if (curLine[0] == '#') // comment
                {
                    continue;
                }
                else if (curLine[0] == 'v')
                {
                    if (curLine[1] == 't') // texture
                    {
                        str >> tmp; // consume "vt"
                        str >> c1 >> c2;
                        gTextures.push_back(Texture(c1, c2));
                    }
                    else if (curLine[1] == 'n') // normal
                    {
                        str >> tmp; // consume "vn"
                        str >> c1 >> c2 >> c3;
                        gNormals.push_back(Normal(c1, c2, c3));
                    }
                    else // vertex
                    {
                        str >> tmp; // consume "v"
                        str >> c1 >> c2 >> c3;
                        gVertices.push_back(Vertex(c1, c2, c3));
                    }
                }
                else if (curLine[0] == 'f') // face
                {
                    str >> tmp; // consume "f"
					char c;
					int vIndex[3],  nIndex[3], tIndex[3];
					str >> vIndex[0]; str >> c >> c; // consume "//"
					str >> nIndex[0]; 
					str >> vIndex[1]; str >> c >> c; // consume "//"
					str >> nIndex[1]; 
					str >> vIndex[2]; str >> c >> c; // consume "//"
					str >> nIndex[2]; 

					assert(vIndex[0] == nIndex[0] &&
						   vIndex[1] == nIndex[1] &&
						   vIndex[2] == nIndex[2]); // a limitation for now

					// make indices start from 0
					for (int c = 0; c < 3; ++c)
					{
						vIndex[c] -= 1;
						nIndex[c] -= 1;
						tIndex[c] -= 1;
					}

                    gFaces.push_back(Face(vIndex, tIndex, nIndex));
                }
                else
                {
                    cout << "Ignoring unidentified line in obj file: " << curLine << endl;
                }
            }

            //data += curLine;
            if (!myfile.eof())
            {
                //data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }

	/*
	for (int i = 0; i < gVertices.size(); ++i)
	{
		Vector3 n;

		for (int j = 0; j < gFaces.size(); ++j)
		{
			for (int k = 0; k < 3; ++k)
			{
				if (gFaces[j].vIndex[k] == i)
				{
					// face j contains vertex i
					Vector3 a(gVertices[gFaces[j].vIndex[0]].x, 
							  gVertices[gFaces[j].vIndex[0]].y,
							  gVertices[gFaces[j].vIndex[0]].z);

					Vector3 b(gVertices[gFaces[j].vIndex[1]].x, 
							  gVertices[gFaces[j].vIndex[1]].y,
							  gVertices[gFaces[j].vIndex[1]].z);

					Vector3 c(gVertices[gFaces[j].vIndex[2]].x, 
							  gVertices[gFaces[j].vIndex[2]].y,
							  gVertices[gFaces[j].vIndex[2]].z);

					Vector3 ab = b - a;
					Vector3 ac = c - a;
					Vector3 normalFromThisFace = (ab.cross(ac)).getNormalized();
					n += normalFromThisFace;
				}

			}
		}

		n.normalize();

		gNormals.push_back(Normal(n.x, n.y, n.z));
	}
	*/

	assert(gVertices.size() == gNormals.size());

    return true;
}

bool ReadDataFromFile(
    const string& fileName, ///< [in]  Name of the shader file
    string&       data)     ///< [out] The contents of the file
{
    fstream myfile;

    // Open the input 
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            data += curLine;
            if (!myfile.eof())
            {
                data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }

    return true;
}

void createVS(GLuint& program, const string& filename)
{
    string shaderSource;

    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &shader, &length);
    glCompileShader(vs);

    char output[1024] = {0};
    glGetShaderInfoLog(vs, 1024, &length, output);
    printf("VS compile log: %s\n", output);

    glAttachShader(program, vs);
}

void createFS(GLuint& program, const string& filename)
{
    string shaderSource;

    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &shader, &length);
    glCompileShader(fs);

    char output[1024] = {0};
    glGetShaderInfoLog(fs, 1024, &length, output);
    printf("FS compile log: %s\n", output);

    glAttachShader(program, fs);
}

void initShaders()
{
    gProgram[0] = glCreateProgram();
    gProgram[1] = glCreateProgram();
    gProgram[2] = glCreateProgram();

    createVS(gProgram[0], "vert0.glsl");
    createFS(gProgram[0], "frag0.glsl");

    createVS(gProgram[1], "vert1.glsl");
    createFS(gProgram[1], "frag1.glsl");

    createVS(gProgram[2], "vert_text.glsl");
    createFS(gProgram[2], "frag_text.glsl");

    glBindAttribLocation(gProgram[0], 0, "inVertex");
    glBindAttribLocation(gProgram[0], 1, "inNormal");
    glBindAttribLocation(gProgram[1], 0, "inVertex");
    glBindAttribLocation(gProgram[1], 1, "inNormal");
    glBindAttribLocation(gProgram[2], 2, "vertex");

    glLinkProgram(gProgram[0]);
    glLinkProgram(gProgram[1]);
    glLinkProgram(gProgram[2]);
    glUseProgram(gProgram[0]);

    gIntensityLoc = glGetUniformLocation(gProgram[0], "intensity");
    cout << "gIntensityLoc = " << gIntensityLoc << endl;
    glUniform1f(gIntensityLoc, gIntensity);
}

void initVBO()
{
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    assert(glGetError() == GL_NONE);

    glGenBuffers(1, &gVertexAttribBuffer);
    glGenBuffers(1, &gIndexBuffer);

    assert(gVertexAttribBuffer > 0 && gIndexBuffer > 0);

    glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

    gVertexDataSizeInBytes = gVertices.size() * 3 * sizeof(GLfloat);
    gNormalDataSizeInBytes = gNormals.size() * 3 * sizeof(GLfloat);
    int indexDataSizeInBytes = gFaces.size() * 3 * sizeof(GLuint);
    GLfloat* vertexData = new GLfloat [gVertices.size() * 3];
    GLfloat* normalData = new GLfloat [gNormals.size() * 3];
    GLuint* indexData = new GLuint [gFaces.size() * 3];

    float minZ = 1e6, maxZ = -1e6;

    for (int i = 0; i < gVertices.size(); ++i)
    {
        vertexData[3*i] = gVertices[i].x;
        vertexData[3*i+1] = gVertices[i].y;
        vertexData[3*i+2] = gVertices[i].z;

        avgX+=gVertices[i].x;
        avgY+=gVertices[i].y;
        
        minX = std::min(minX, gVertices[i].x);
        maxX = std::max(maxX, gVertices[i].x);
        minY = std::min(minY, gVertices[i].y);
        maxY = std::max(maxY, gVertices[i].y);
        minZ = std::min(minZ, gVertices[i].z);
        maxZ = std::max(maxZ, gVertices[i].z);
    }
    
    avgX/=gVertices.size();
    avgY/=gVertices.size();


    for (int i = 0; i < gNormals.size(); ++i)
    {
        normalData[3*i] = gNormals[i].x;
        normalData[3*i+1] = gNormals[i].y;
        normalData[3*i+2] = gNormals[i].z;
    }

    for (int i = 0; i < gFaces.size(); ++i)
    {
        indexData[3*i] = gFaces[i].vIndex[0];
        indexData[3*i+1] = gFaces[i].vIndex[1];
        indexData[3*i+2] = gFaces[i].vIndex[2];
    }


    glBufferData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes + gNormalDataSizeInBytes, 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, gVertexDataSizeInBytes, vertexData);
    glBufferSubData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes, gNormalDataSizeInBytes, normalData);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSizeInBytes, indexData, GL_STATIC_DRAW);

    // done copying; can free now
    delete[] vertexData;
    delete[] normalData;
    delete[] indexData;

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));

}

void initFonts(int windowWidth, int windowHeight)
{
    // Set OpenGL options
    //glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(windowWidth), 0.0f, static_cast<GLfloat>(windowHeight));
    glUseProgram(gProgram[2]);
    glUniformMatrix4fv(glGetUniformLocation(gProgram[2], "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // FreeType
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    }

    // Load font as face
    FT_Face face;
    if (FT_New_Face(ft, "/usr/share/fonts/truetype/liberation/LiberationSerif-Italic.ttf", 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    }

    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

    // Load first 128 characters of ASCII set
    for (GLubyte c = 0; c < 128; c++)
    {
        // Load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
                );
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    //
    // Configure VBO for texture quads
    //
    glGenBuffers(1, &gTextVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
bool compareVec3(glm::vec3 v1, glm::vec3 v2){
    if (v1[0]==v2[0] && v1[1]==v2[1] && v1[2]==v2[2]){
        return true;
    }
    return false;
}

std::vector<std::pair<int,int>> checkAutoMatch(){
    // cout<<"in chech match"<<endl;
    std::vector<std::pair<int,int>> result;
    //         cout<<"----------"<<endl;
    // for (int i=0; i < row_count; i++) {
    //     for (int j= 0; j < column_count; j++) {
    //         cout<<mapExplosionObj[i][j]<<" ";
    //     }
    //     cout<<endl;
    // }
    //         cout<<"----------"<<endl;
    for (int i=0; i<initialColors.size(); i++){
        for (int j=0; j<initialColors[0].size()-2; j++){
            if (compareVec3(initialColors[i][j],initialColors[i][j+1]) && compareVec3(initialColors[i][j+1],initialColors[i][j+2]) ) {
                mapExplosionObj[row_count - i - 1][j]=1;
                mapExplosionObj[row_count - i - 1][j+1]=1;
                mapExplosionObj[row_count - i - 1][j+2]=1;
                result.push_back({i, j});
                result.push_back({i, j + 1});
                result.push_back({i, j + 2});
            }
        }
    }
    for (int j=0; j < initialColors[0].size(); j++) {
        for (int i= 0; i < initialColors.size() - 2; i++) {
            if (compareVec3(initialColors[i][j],initialColors[i+1][j]) && compareVec3(initialColors[i][j],initialColors[i+2][j])) {
                mapExplosionObj[row_count - i - 1][j]=1;
                mapExplosionObj[row_count - i - 1 - 1][j]=1;
                mapExplosionObj[row_count - i - 2 - 1][j]=1;
                result.push_back({i, j});
                result.push_back({i + 1, j});
                result.push_back({i + 2, j});
            }
        }
    }
    return result;

}
void init(char* objectText) 
{
	
	ParseObj(objectText);

    glEnable(GL_DEPTH_TEST);
    initShaders();
    initFonts(gWidth, gHeight);
    initVBO();
}

void drawModel()
{
	glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));

	glDrawElements(GL_TRIANGLES, gFaces.size() * 3, GL_UNSIGNED_INT, 0);
}

void renderText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
    // Activate corresponding render state	
    glUseProgram(gProgram[2]);
    glUniform3f(glGetUniformLocation(gProgram[2], "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) 
    {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        // Update VBO for each character
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },            
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }           
        };

        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

        //glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)

        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}




void updateColor(){

    for (int j=0; j<column_count; j++){
        int count=0;
        vector<glm::vec3> queue;
        vector<int> queueForMoving;
        for (int i=row_count-1; i>-1; i--){
            if (mapExplosionObj[i][j]==0){
                queue.insert(queue.begin(),initialColors[row_count-i-1][j]);
                queueForMoving.insert(queueForMoving.begin(),count);
            }
            else {
                count++;
            }
        }
        for (int i=0; i<count; i++){
            std::random_device rd; 
            std::mt19937 eng(rd()); 
            std::uniform_int_distribution<> distr(0, randomColors.size()-1);
            int a=distr(eng);
            queue.insert(queue.begin(),randomColors[a]);
            queueForMoving.insert(queueForMoving.begin(),count);
        }
        for (int i=0; i<row_count; i++){
            initialColors[row_count-i-1][j]=queue[i];
            temporaryLocationChange[i][j]=queueForMoving[i];
        }
    }

}

void explodedObjectsFinder(){ 
    
    for (int j=0; j< column_count; j++){
        int count = 0;
        bool on_explosion= false;
        int min_index = -1;
        for (int i=0; i<row_count; i++){
            if (mapExplosionObj[i][j]==1){
                count++;
                if(on_explosion == false){// ilk defa girecegi tek yer
                    min_index = i;
                    on_explosion = true;
                }
                if(i == row_count -1){
                    SlidingValues tObj= SlidingValues();
                    tObj.columnIndex = j;
                    tObj.rowMinIndex = min_index;
                    tObj.rowMaxIndex=i;
                    tObj.count=count;
                    sliding_objects.push_back(tObj);
                }

            }else{
                if(on_explosion == true){
                    SlidingValues tObj= SlidingValues();
                    tObj.columnIndex = j;
                    tObj.rowMaxIndex= (i-1);
                    tObj.count=count;
                    tObj.rowMinIndex = min_index;
                    sliding_objects.push_back(tObj);
                    on_explosion = false;
                    count = 0;
                    min_index = -1;
                }
            }
        }
    }
    
} 

void implement_steady_state(){
                                            
    glm::mat4 perspMat = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -20.0f, 20.0f);
    float height=20.0f, width=20.0f;
    float sX=width/column_count/(maxX-minX);
    float sY=height/row_count/(maxY-minY);
    // clear bitmap 
    for(int i = 0; i<row_count; i++){
        for(int j = 0; j< column_count; j++){
            mapExplosionObj[i][j] = 0;
            columnCountExploded[j]=0;
        }
    }
    
    
    // same rotation for all
    glm::mat4 RotateMat = glm::rotate(glm::mat4(1.f), glm::radians(-angle), glm::vec3(0, 1, 0));
    
    // now going on for different translations through the grids via for loop
    for(int i = 0 ; i < row_count; i++ ){
        for(int j = 0; j< column_count; j++){
                glUseProgram(gProgram[0]);

                // same scaling option for all vertices
                glm::mat4 ScaleMat= glm::scale(glm::mat4(1.f),glm::vec3(sX,sY,1));

                glm::mat4 TranslationMat = glm::translate(glm::mat4(1.f), glm::vec3(-10+width/column_count*j+width/(2*column_count)
                                                                                    ,-10+height/row_count*i+height/(2*row_count)
                                                                                    ,-10.0f));
                glm::mat4 ModellingMat=TranslationMat*ScaleMat*RotateMat;

                glm::mat4 modelMatInv = glm::transpose(glm::inverse(ModellingMat));
            
                GLint uniformLocation = glGetUniformLocation(gProgram[0], "kd");
                glUniform3f(uniformLocation, initialColors[i][j][0], initialColors[i][j][1], initialColors[i][j][2]);     
                glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMat"), 1, GL_FALSE, glm::value_ptr(ModellingMat));
                glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
                glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "perspectiveMat"), 1, GL_FALSE, glm::value_ptr(perspMat));
                drawModel();
        }
    }                
    anyAutoMatches=checkAutoMatch();
    
    // for(int i = 0; i < values.size(); i++){
    //     cout<<"----"<<endl;
    //     cout<<values[i].columnIndex<<" "<<values[i].count<<" "<<values[i].rowMaxIndex<<" "<<values[i].rowMinIndex<<endl;
    //     cout<<"----"<<endl;
    // }
    // cout<<anyAutoMatches.size()<<endl;
    if (anyAutoMatches.size() != 0) { // match found
        explodedObjectsFinder();
        
        for (int j=0; j<column_count; j++){
            int tmp=0;
            for (int i=0; i<row_count;i++){
                if (mapExplosionObj[i][j]==1){
                    totalCountGonnaExplode++;
                    tmp++;
                }
                
            }
            columnCountExploded[j]=tmp;
        }
        state = ANY_MATCH;
    }
}

void implement_any_match(){
                                            
    glm::mat4 perspMat = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -20.0f, 20.0f);
    float height=20.0f, width=20.0f;
    float sX=width/column_count/(maxX-minX);
    float sY=height/row_count/(maxY-minY);
    explosionRate+=0.01f;
    // same rotation for all
    glm::mat4 RotateMat = glm::rotate(glm::mat4(1.f), glm::radians(-angle), glm::vec3(0, 1, 0));
    
    // now going on for different translations through the grids via for loop
    for(int i = 0 ; i < row_count; i++ ){
        for(int j = 0; j< column_count; j++){
                glUseProgram(gProgram[0]);

                // same scaling option for all vertices
                glm::mat4 ScaleMat= glm::scale(glm::mat4(1.f),glm::vec3(sX,sY,1));
                
                // if there is auto match explode them 
                // this is buggy it grows faster than the clicking need a check
                for (int k=0; k<anyAutoMatches.size(); k++){
                    
                    if (i==anyAutoMatches[k].first && j==anyAutoMatches[k].second){

                        
                                                  
                        ScaleMat= glm::scale(glm::mat4(1.f),glm::vec3(sX*explosionRate,sY*explosionRate,1));
                        
                        if (explosionRate>=1.5f){
                            explosionRate=1.0f;
                            // tempCount++;
                            // cout<<"Temp count"<<endl;
                            // cout<<tempCount<<endl;
                            // cout<<"totalCountGonnaExplode"<<endl;
                            // cout<<totalCountGonnaExplode<<endl;
                            totalPoints+=totalCountGonnaExplode;
                            state=ANIMATION;
                            tempCount=0;
                            totalCountGonnaExplode=0;
                            // if (tempCount==totalCountGonnaExplode){
                                
                            // }
                            
                            
                        }
                    }
                }
                glm::mat4 TranslationMat = glm::translate(glm::mat4(1.f), glm::vec3(-10+width/column_count*j+width/(2*column_count)
                                                                                    ,-10+height/row_count*i+height/(2*row_count)
                                                                                    ,-10.0f));
                glm::mat4 ModellingMat=TranslationMat*ScaleMat*RotateMat;
                
                glm::mat4 modelMatInv = glm::transpose(glm::inverse(ModellingMat));
            
                GLint uniformLocation = glGetUniformLocation(gProgram[0], "kd");
                glUniform3f(uniformLocation, initialColors[i][j][0], initialColors[i][j][1], initialColors[i][j][2]);     
                glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMat"), 1, GL_FALSE, glm::value_ptr(ModellingMat));
                glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
                glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "perspectiveMat"), 1, GL_FALSE, glm::value_ptr(perspMat));
                
                drawModel();
        }
    }                

}

void implement_button_clicked(){
                                            
    glm::mat4 perspMat = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -20.0f, 20.0f);
    float height=20.0f, width=20.0f;
    float sX=width/column_count/(maxX-minX);
    float sY=height/row_count/(maxY-minY);
    
    // same rotation for all
    glm::mat4 RotateMat = glm::rotate(glm::mat4(1.f), glm::radians(-angle), glm::vec3(0, 1, 0));

    // now going on for different translations through the grids via for loop
    for(int i = 0 ; i < row_count; i++ ){
        for(int j = 0; j< column_count; j++){
                glUseProgram(gProgram[0]);

                // same scaling option for all vertices
                glm::mat4 ScaleMat= glm::scale(glm::mat4(1.f),glm::vec3(sX,sY,1));
                // if explosion is handled by clicking button 
                if ((xpos*20.0f/gWidth)<=(j+1)*width/column_count && (xpos*20.0f/gWidth)>=(j)*width/column_count
                                            &&  (ypos*20.0f/gHeight)<=(row_count-i)*height/row_count && (ypos*20.0f/gHeight)>=(row_count-i-1)*height/row_count){
                    
                    explosionRate+=0.01f;
                                                  
                    ScaleMat= glm::scale(glm::mat4(1.f),glm::vec3(sX*explosionRate,sY*explosionRate,1));

                    if (explosionRate>=1.5f){
                        
                        explosionRate=1.0f;
                        state = ANIMATION;
                        mapExplosionObj[row_count-i-1][j]=1;
                        continue;
                    }
                }
                glm::mat4 TranslationMat = glm::translate(glm::mat4(1.f), glm::vec3(-10+width/column_count*j+width/(2*column_count)
                                                                                    ,-10+height/row_count*i+height/(2*row_count)
                                                                                    ,-10.0f));
                glm::mat4 ModellingMat=TranslationMat*ScaleMat*RotateMat;

                glm::mat4 modelMatInv = glm::transpose(glm::inverse(ModellingMat));
            
                GLint uniformLocation = glGetUniformLocation(gProgram[0], "kd");
                glUniform3f(uniformLocation, initialColors[i][j][0], initialColors[i][j][1], initialColors[i][j][2]);     
                glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMat"), 1, GL_FALSE, glm::value_ptr(ModellingMat));
                glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
                glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "perspectiveMat"), 1, GL_FALSE, glm::value_ptr(perspMat));
                drawModel();
        }
    }                
}

void implement_animation(){

    glm::mat4 perspMat = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -20.0f, 20.0f);
    float height=20.0f, width=20.0f;
    float sX=width/column_count/(maxX-minX);
    float sY=height/row_count/(maxY-minY);
    bool flag=true;

    // same rotation for all
    glm::mat4 RotateMat = glm::rotate(glm::mat4(1.f), glm::radians(-angle), glm::vec3(0, 1, 0));
    slidingRate-=0.05f;

    // now going on for different translations through the grids via for loop
    for (int j=0; j<column_count; j++){

        int columnCount=columnCountExploded[j];
        for (int i=0; i<row_count; i++){

            glUseProgram(gProgram[0]);


            int locationChanged=temporaryLocationChange[row_count-i-1][j];

            float remaining=locationChanged*height/row_count+slidingRate;
            // cout<<"----"<<endl;
            // cout<<remaining<<endl;

            glm::mat4 TranslationMat;
            if (remaining<=0){
                TranslationMat=glm::translate(glm::mat4(1.f), glm::vec3(-10+width/column_count*j+width/(2*column_count)
                                                                                ,-10+height/row_count*i+height/(2*row_count)
                                                                                ,-10.0f));
            }
            else{
                flag=false;
                TranslationMat = glm::translate(glm::mat4(1.f), glm::vec3(-10+width/column_count*j+width/(2*column_count)
                                                                                ,-10+height/row_count*i+height/(2*row_count)+remaining
                                                                                ,-10.0f));
            }
            
            
            // same scaling option for all vertices
            glm::mat4 ScaleMat= glm::scale(glm::mat4(1.f),glm::vec3(sX,sY,1));

            
            glm::mat4 ModellingMat=TranslationMat*ScaleMat*RotateMat;

            glm::mat4 modelMatInv = glm::transpose(glm::inverse(ModellingMat));
        
            GLint uniformLocation = glGetUniformLocation(gProgram[0], "kd");
            glUniform3f(uniformLocation, initialColors[i][j][0], initialColors[i][j][1], initialColors[i][j][2]);     
            glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMat"), 1, GL_FALSE, glm::value_ptr(ModellingMat));
            glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
            glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "perspectiveMat"), 1, GL_FALSE, glm::value_ptr(perspMat));
            drawModel();
        }
    }
    if (flag){
        state=STEADY_STATE;
        slidingRate=0;
    }
    


}



void display()
{
    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    // cout<<ButtonClicked<<endl;
    // cout<<xpos<<endl;
    // cout<<ypos<<endl;

    if(state == STEADY_STATE ){
        implement_steady_state();
    } else if( state == ANY_MATCH ){
        implement_any_match();
        if (state == ANIMATION){
            updateColor();
        }

    } else if( state == BUTTON_CLICKED ){
        
        implement_button_clicked();
        if (state == ANIMATION){
            updateColor();
        }
    }else if ( state == ANIMATION ){
        implement_animation();
    }
    

    assert(glGetError() == GL_NO_ERROR);

    //Render the moves and points
    char text[100]={};
    sprintf(text,"Moves:%d  Score:%d",moves,totalPoints );
    renderText(text, 1, 1, 1, glm::vec3(1, 1, 1));

    assert(glGetError() == GL_NO_ERROR);
	angle += 0.5;
}

void reshape(GLFWwindow* window, int w, int h)
{
    w = w < 1 ? 1 : w;
    h = h < 1 ? 1 : h;

    gWidth = w;
    gHeight = h;

    glViewport(0, 0, w, h);
}
void mainLoop(GLFWwindow* window)
{
    moves=0;
    totalPoints=0;
    initialColors.resize(row_count);
    mapExplosionObj.resize(row_count);
    columnCountExploded.resize(column_count);
    temporaryLocationChange.resize(row_count);
    for (int i=0; i<row_count;i++){
        initialColors[i].resize(column_count);
        mapExplosionObj[i].resize(column_count);
        temporaryLocationChange[i].resize(column_count);
        for (int j=0; j<column_count; j++){


            std::random_device rd; 
            std::mt19937 eng(rd()); 
            std::uniform_int_distribution<> distr(0, randomColors.size()-1);
            int a=distr(eng);


            initialColors[i][j]=randomColors[a];
            mapExplosionObj[i][j]=0;
        }
    }

    while (!glfwWindowShouldClose(window))
    {
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        cout<<"SHUTTING DOWN" << endl;
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    else if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        cout << "F pressed" << endl;
        glUseProgram(gProgram[1]);
    }
    else if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        cout<<"RESTARTING THE GAME"<<endl;
        mainLoop(window);
    }
    else if (key == GLFW_KEY_V && action == GLFW_PRESS)
    {
        cout << "V pressed" << endl;
        glUseProgram(gProgram[0]);
    }
    else if (key == GLFW_KEY_D && action == GLFW_PRESS)
    {
        cout << "D pressed" << endl;
        gIntensity /= 1.5;
        cout << "gIntensity = " << gIntensity << endl;
        glUseProgram(gProgram[0]);
        glUniform1f(gIntensityLoc, gIntensity);
    }
    else if (key == GLFW_KEY_B && action == GLFW_PRESS)
    {
        cout << "B pressed" << endl;
        gIntensity *= 1.5;
        cout << "gIntensity = " << gIntensity << endl;
        glUseProgram(gProgram[0]);
        glUniform1f(gIntensityLoc, gIntensity);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && state==STEADY_STATE) 
    {
        state = BUTTON_CLICKED;
        moves++;
       //getting cursor position
       glfwGetCursorPos(window, &xpos, &ypos);
       
    }
}



int main(int argc, char * argv[]) 
{
    state = STEADY_STATE;
    GLFWwindow* window;
    if(argc != 4){
        cout<<"Not enough inputs!!!!"<<endl;
    }
    column_count=std::stoi(argv[1]);
    row_count=std::stoi(argv[2]);
    char* objectText=argv[3];
    if (!glfwInit())
    {
        exit(-1);
    }
    

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(gWidth, gHeight, "Simple Example", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialize GLEW to setup the OpenGL Function pointers
    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return EXIT_FAILURE;
    }

    char rendererInfo[512] = {0};
    strcpy(rendererInfo, (const char*) glGetString(GL_RENDERER));
    strcat(rendererInfo, " - ");
    strcat(rendererInfo, (const char*) glGetString(GL_VERSION));
    glfwSetWindowTitle(window, rendererInfo);


    init(objectText);

    
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window,keyboard);
    glfwSetWindowSizeCallback(window, reshape);

    reshape(window, gWidth, gHeight); // need to call this once ourselves
    mainLoop(window); // this does not return unless the window is closed

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

