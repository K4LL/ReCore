#pragma once
#include "framework.h"

#include <io.h>
#include <fstream>
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>

struct ModelMeta {
	std::string texturePath;
	std::string modelPath;

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 rotation;
	DirectX::XMFLOAT3 scale;
};

class MetaIO {
private:
	std::vector<ModelMeta> models;

	void parsePosition(ModelMeta* model, std::string* line);
	void parseRotation(ModelMeta* model, std::string* line);
	void parseScale(ModelMeta* model, std::string* line);

public:
	MetaIO() = default;

	void read(const std::string& path);
	void write(const std::string& path, const ModelMeta& model);
	void clear(const std::string& path);

	void printCode();

	void print();
};