#include "Meta.h"

void MetaIO::parsePosition(ModelMeta* model, std::string* line) {
	switch ((*line)[1]) {
	case 'x': {
		model->position.x = std::stof(line->erase(0, 2));
		break;
	}
	case 'y': {
		model->position.y = std::stof(line->erase(0, 2));
		break;
	}
	case 'z': {
		model->position.z = std::stof(line->erase(0, 2));
		break;
	}

	default:
		break;
	}
}
void MetaIO::parseRotation(ModelMeta* model, std::string* line) {
	switch ((*line)[1]) {
	case 'x': {
		model->rotation.x = std::stof(line->erase(0, 2));
		break;
	}
	case 'y': {
		model->rotation.y = std::stof(line->erase(0, 2));
		break;
	}
	case 'z': {
		model->rotation.z = std::stof(line->erase(0, 2));
		break;
	}

	default:
		break;
	}
}
void MetaIO::parseScale(ModelMeta* model, std::string* line) {
	switch ((*line)[1]) {
	case 'x': {
		model->scale.x = std::stof(line->erase(0, 2));
		break;
	}
	case 'y': {
		model->scale.y = std::stof(line->erase(0, 2));
		break;
	}
	case 'z': {
		model->scale.z = std::stof(line->erase(0, 2));
		break;
	}

	default:
		break;
	}
}

void MetaIO::read(const std::string& path) {
	std::ifstream file(path);
	RC_EIR_ASSERT(!file.is_open(), "Failed to open file for reading! File path:" << path);

	std::string line;
	ModelMeta   model = {};
	while (std::getline(file, line)) {
		switch (line[0]) {
		case 't': {
			std::string str = line.erase(0, 1);
			std::string copy;
			for (auto& c : str) {
				if (c == '\\') {
					copy += "\\\\";
				}
				else {
					copy += c;
				}
			}

			model.texturePath = copy;
			break;
		}
		case 'm': {
			if (!model.modelPath.empty()) {
				this->models.push_back(model);
				model = {};
			}

			std::string str = line.erase(0, 1);
			std::string copy;
			for (auto& c : str) {
				if (c == '\\') {
					copy += "\\\\";
				}
				else {
					copy += c;
				}
			}

			model.modelPath = copy;
			break;
		}
		case 'p': {
			parsePosition(&model, &line);
			break;
		}
		case 'r': {
			parseRotation(&model, &line);
			break;
		}
		case 's': {
			parseScale(&model, &line);
			break;
		}

		default:
			break;
		}
	}

	this->models.push_back(model);
}
void MetaIO::write(const std::string& path, const ModelMeta& model) {
	std::fstream file(path, std::ios::in | std::ios::out);
	RC_EIR_ASSERT(!file.is_open(), "Failed to open file for writing! File path: " << path);

	std::ofstream outFile(path, std::ios::app);

	outFile << 'm' << model.modelPath << '\n';
	outFile << 't' << model.texturePath << '\n';

	outFile << "px" << model.position.x << '\n';
	outFile << "py" << model.position.y << '\n';
	outFile << "pz" << model.position.z << '\n';

	outFile << "rx" << model.rotation.x << '\n';
	outFile << "ry" << model.rotation.y << '\n';
	outFile << "rz" << model.rotation.z << '\n';

	outFile << "sx" << model.scale.x << '\n';
	outFile << "sy" << model.scale.y << '\n';
	outFile << "sz" << model.scale.z << '\n';
}

void MetaIO::clear(const std::string& path) {
	std::ofstream file(path, std::ios::trunc);
	RC_EI_ASSERT(!file.is_open(), "Failed to clear the file: " << path);
}

void MetaIO::printCode() {
	for (unsigned int i = 0; i < this->models.size(); i++) {
		std::cout << std::format(R"(
			Model model{} = std::move(renderer.createModel("{}", 
									GroundVertexShaderSource, 
									GroundPixelShaderSource, 
									"{}"));
			model{}.transform.position = DirectX::XMVectorSet({}, {}, {}, 1.0f);
			model{}.transform.rotation = DirectX::XMVectorSet({}, {}, {}, 1.0f);
			model{}.transform.scale    = DirectX::XMVectorSet({}, {}, {}, 1.0f);

			renderer.toQueue(std::move(model{}));
			)",
			i,
			models[i].modelPath, models[i].texturePath,
			i,
			models[i].position.x, models[i].position.y, models[i].position.z,
			i,
			models[i].rotation.x, models[i].rotation.y, models[i].rotation.z,
			i,
			models[i].scale.x, models[i].scale.y, models[i].scale.z,
			i
		) << '\n';
	}
}

void MetaIO::print() {
	RC_WIR_ASSERT(this->models.empty(), "MetaIO print called, but no meta file was loaded!");

	for (auto& model : this->models) {
		RC_DBG_LOG("Model path: " << model.modelPath);
		RC_DBG_LOG("Texture path: " << model.texturePath);

		RC_DBG_LOG(std::format("Position:\n- X: {}\n- Y: {}\n- Z: {}",
			model.position.x, model.position.y, model.position.z));
		RC_DBG_LOG(std::format("Rotation:\n- X: {}\n- Y: {}\n- Z: {}",
			model.rotation.x, model.rotation.y, model.rotation.z));
		RC_DBG_LOG(std::format("Scale:\n- X: {}\n- Y: {}\n- Z: {}",
			model.scale.x, model.scale.y, model.scale.z));
	}
}