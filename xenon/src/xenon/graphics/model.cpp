#include "model.h"

#include "xenon/core/log.h"
#include "xenon/graphics/model_loader.h"

namespace xe {

	void destroyModel(Model* model) {
		XE_LOG_TRACE_F("MODEL: Destroying model: {}", model->metadata.path);

		for (const Primitive& primitive : model->primitives) {
			glDeleteVertexArrays(1, &primitive.vao);
			if (primitive.ebo) {
				glDeleteBuffers(1, &primitive.ebo);
			}
		}

		for (const PrimitiveAttributeArray& attributeArray : model->primitiveAttributes) {
			for (const PrimitiveAttribute& attribute : attributeArray) {
				if (attribute.vbo) {
					glDeleteBuffers(1, &attribute.vbo);
				}
			}
		}

		delete model;
	}

	void ModelSerializer::serialize(Asset* asset) const {
		// TODO: Implement
		XE_LOG_ERROR("Model serialization is not yet implemented.");
	}

	bool ModelSerializer::loadData(Asset** asset) const {
		const Asset* sourceAsset = *asset;
		*asset = loadModel((*asset)->metadata.path);
		copyAssetMetaRuntimeData(sourceAsset, *asset);
		return true;
	}

}
