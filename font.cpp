#include "font.hpp"

std::unordered_map<int, textbox*> textbox::ownerList;

textbox::textbox(std::wstring text, int x, int y, int size, int r, int g, int b):
											text(text), size(size), x(x), y(y), r(r), g(g), b(b) {
	length = text.length();
	render();
}

void textbox::render() {
	float originX         = ((float)x/(float)windowWidth)*2.0f - 1.0f;
	float originY         = ((float)y/(float)windowWidth)*2.0f - 1.0f;
	float fontScaleWidth  = (float)size/(float)windowWidth * 2;
	float fontScaleHeight = (float)size/(float)windowHeight * 2;

	float normalR = (float)r/255.0f;
	float normalG = (float)g/255.0f;
	float normalB = (float)b/255.0f;

	characterIDArray = std::unique_ptr<int[]>(new int[text.length()]);

	for (int i = 0; i < length; i++) {
		auto info = font::getCharInfo(text[i]);

		float scaledWidth        = fontScaleWidth  * ((float)info.width        / (float)font::textureHeight);
		float scaledHeight       = fontScaleHeight * ((float)info.height       / (float)font::textureHeight);
		float scaledBearingX     = fontScaleWidth  * ((float)info.bearingX     / (float)font::textureHeight);
		float scaledBearingY     = fontScaleHeight * ((float)info.bearingY     / (float)font::textureHeight);
		float scaledAdvanceWidth = fontScaleWidth  * (((float)info.advanceWidth/64.0f) / (float)font::textureHeight);

		float aX = originX + scaledBearingX;
		float aY = originY + scaledBearingY - scaledHeight;
		float eX = aX + scaledWidth;
		float eY = aY + scaledHeight;

		int charID = 	font::addCharacterToDrawList({
									{aX, eY, info.u01, info.v01, normalR, normalG, normalB},
									{aX, aY, info.u00, info.v00, normalR, normalG, normalB},
									{eX, aY, info.u10, info.v10, normalR, normalG, normalB},

									{aX, eY, info.u01, info.v01, normalR, normalG, normalB},
									{eX, aY, info.u10, info.v10, normalR, normalG, normalB},
									{eX, eY, info.u11, info.v11, normalR, normalG, normalB}
								});

		characterIDArray[i] = charID;
		ownerList[charID] = this;

		originX += scaledAdvanceWidth;
	}

	font::reloadVBO();
}

void textbox::updateText(std::wstring newText) {
	destroy();
	text = newText;
	length = newText.length();
	render();
}

void textbox::updateColor(int newR, int newG, int newB) {
	destroy();
	r = newR;
	g = newG;
	b = newB;
	render();
}

void textbox::updateSize(int newSize) {
	destroy();
	size = newSize;
	render();
}

void textbox::updatePos(int newX, int newY) {
	destroy();
	x = newX;
	y = newY;
	render();
}

void textbox::updateID(int before, int after) {
	for (int i = 0; i < length; i++) {
		if (characterIDArray[i] == before) {
			characterIDArray[i] = after;
			return;
		}
	}

	std::cout << "program Error(ecaohfmoefhaouwhf)" << std::endl;
}

void textbox::destroy() {
	for (int i = 0; i < length; i++) {
		int target = characterIDArray[i];
		int alterdID = font::removeCharacterFromDrawList(target);
		ownerList[target] = ownerList[alterdID];
		ownerList[alterdID]->updateID(alterdID, target);
	}
	font::reloadVBO();
}

namespace font {
	FT_Library ft;
	FT_Face face;

	GLuint program_TEXT;
	GLuint characterVAO, characterVBO;

	GLuint textAtlasItr = 0;
	GLuint textAtlasWidth = 512;
	std::vector<character> characterVector;
	std::unordered_map<wchar_t, charInfo> charMap;


	charInfo getCharInfo(wchar_t request) {
		if (auto iter = charMap.find(request); iter != end(charMap)) {
			//found
			return iter->second;
		} else {
			//not found
			FT_Load_Glyph(face, FT_Get_Char_Index(face, request), FT_LOAD_RENDER);
			const auto &glyph = face->glyph;

			while (textAtlasItr + glyph->bitmap.width > textAtlasWidth) {
				expandVBO();
			}

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glBindTexture(GL_TEXTURE_2D, textAtlas);
			glTexSubImage2D(GL_TEXTURE_2D, 0, textAtlasItr, 0, glyph->bitmap.width, glyph->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, glyph->bitmap.buffer);
			charInfo newchar = {(float)textAtlasItr/(float)textAtlasWidth,							(float)glyph->bitmap.rows/(float)textureHeight, //01
								(float)textAtlasItr/(float)textAtlasWidth,							0,												//00
								(float)(textAtlasItr + glyph->bitmap.width)/(float)textAtlasWidth,	(float)glyph->bitmap.rows/(float)textureHeight, //11
								(float)(textAtlasItr + glyph->bitmap.width)/(float)textAtlasWidth,	0,												//10
								(GLuint)glyph->bitmap.width,
								(GLuint)glyph->bitmap.rows,
								(GLuint)glyph->bitmap_left,
								(GLuint)glyph->bitmap_top,
								(GLuint)glyph->advance.x};
			textAtlasItr += glyph->bitmap.width;
			charMap[request] = newchar;
			return newchar;
		}
	}



	void reloadVBO() {
		glBindBuffer(GL_ARRAY_BUFFER, characterVBO);
		glBufferData(GL_ARRAY_BUFFER, characterVector.size() * sizeof(character), &characterVector[0], GL_STATIC_DRAW);
	}


	void expandVBO() {
		glBindTexture(GL_TEXTURE_2D, textAtlas);
		GLubyte *buff = new GLubyte[textAtlasWidth*textureHeight];
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, buff);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, textAtlasWidth*2, textureHeight, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textAtlasWidth, textureHeight, GL_RED, GL_UNSIGNED_BYTE, buff);
		delete buff;

		for (auto& character : characterVector) {
			character.v0.u /= 2.0f;
			character.v1.u /= 2.0f;
			character.v2.u /= 2.0f;
			character.v3.u /= 2.0f;
			character.v4.u /= 2.0f;
			character.v5.u /= 2.0f;
		}

		for (auto itr = charMap.begin(); itr != charMap.end(); itr++) {
			auto& elem = itr->second;
			elem.u00 /= 2;
			elem.u01 /= 2;
			elem.u10 /= 2;
			elem.u11 /= 2;
		}

		textAtlasWidth *= 2;

		reloadVBO();
	}

	int addCharacterToDrawList(character request) {
		int id = characterVector.size();
		characterVector.push_back(request);
		return id;
	}

	int removeCharacterFromDrawList(int id) {
		characterVector[id] = characterVector.back();
		characterVector.pop_back();
		return characterVector.size();
	}


	void setup() {
		if (FT_Init_FreeType(&ft))
			std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
		if (FT_New_Face(ft, "/System/Library/Fonts/ヒラギノ角ゴシック W4.ttc", 0, &face))
			std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;  

		FT_Set_Pixel_Sizes(face, 0, textureHeight);


		glGenTextures(1, &textAtlas);
		glBindTexture(GL_TEXTURE_2D, textAtlas);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, textAtlasWidth, textureHeight, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


		program_TEXT = LoadShaders("text.vert", "text.frag");


		// screen quad VAO
		glGenVertexArrays(1, &characterVAO);
		glGenBuffers(1, &characterVBO);
		glBindVertexArray(characterVAO);
		glBindBuffer(GL_ARRAY_BUFFER, characterVBO);
		glBufferData(GL_ARRAY_BUFFER, characterVector.size() * sizeof(character), &characterVector[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(xyuvrgb), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(xyuvrgb), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(xyuvrgb), (void*)(4 * sizeof(float)));

		textbox hoge0 = textbox(L"柳暗花明又一村", 1024/2 - 210, 768/2 + 200, 60, 255, 255, 255);
		textbox hoge2 = textbox(L"これよこれ This is what I wanted.", 1024/2 - 480, 768/2 - 200, 60, 50, 255, 100);

		textbox hoge1 = textbox(L"柳暗花明又一村", 1024/2 - 355, 768/2, 100, 255, 255, 100);

		hoge1.updateText(L"日本語ですあいうえおかきくけこさしすせそたちつてと");
		hoge1.updateSize(10);

	}

	void draw() {
		glUseProgram(program_TEXT);

		glBindVertexArray(characterVAO);
		glBindTexture(GL_TEXTURE_2D, textAtlas);
		glDrawArrays(GL_TRIANGLES, 0, characterVector.size() * 6);
	}
}


