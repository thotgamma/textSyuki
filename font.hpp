#ifndef FONT_HPP
#define FONT_HPP

#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "shader.hpp"





extern GLuint textAtlas;

class textbox {
	const int windowWidth = 1024;
	const int windowHeight = 768;

	static std::unordered_map<int, textbox*> ownerList;

	std::wstring text;
	int x;
	int y;
	int size;
	int r;
	int g;
	int b;
	int length;

	std::unique_ptr<int[]> characterIDArray;

	public:
	textbox(std::wstring text, int x, int y, int size, int r, int g, int b);
	void updateText(std::wstring newText);
	void updateColor(int newR, int newG, int newB);
	void updateSize(int newSize);
	void updatePos(int newX, int newY);

	void render();
	void updateID(int before, int after);
	void destroy();
};


namespace font {

	struct xyuvrgb {
		float x;
		float y;
		float u;
		float v;
		float r;
		float g;
		float b;
	};

	struct character {
		xyuvrgb v0;
		xyuvrgb v1;
		xyuvrgb v2;
		xyuvrgb v3;
		xyuvrgb v4;
		xyuvrgb v5;
	};

	struct charInfo {
		//10 11
		//00 01
		float u00;
		float v00;
		float u01;
		float v01;
		float u10;
		float v10;
		float u11;
		float v11;
		unsigned int width;
		unsigned int height;
		unsigned int bearingX;
		unsigned int bearingY;
		unsigned int advanceWidth;
	};


	const GLuint textureHeight = 128;

	extern FT_Library ft;
	extern FT_Face face;

	extern GLuint program_TEXT;
	extern GLuint characterVAO, characterVBO;

	extern GLuint textAtlasItr;
	extern GLuint textAtlasWidth;
	extern std::vector<character> characterVector;
	extern std::unordered_map<wchar_t, charInfo> charMap;



	//字形情報を返す。フォントレンダからの字形登録までやってくれる。
	charInfo getCharInfo(wchar_t request);
	void reloadVBO();
	void expandVBO();
	int addCharacterToDrawList(character request);
	int removeCharacterFromDrawList(int id);
	void setup();
	void draw();

}


#endif
