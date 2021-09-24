#include "model_inspector.h"

#include <imgui.h>

namespace xe {

	bool drawModelInspector(const Model& model) {
		bool open = ImGui::Begin("Model inspector");
		if (open) {

			size_t primitiveCounter = 0;
			for (size_t i = 0; i < model.nodes.size(); ++i) {
				const ModelNode& node = model.nodes[i];

				if (ImGui::TreeNode(("Node " + std::to_string(i)).c_str())) {
					ImGui::Text(("Parent: " + std::to_string(node.parent)).c_str());

					// pii = primitiveIndicesIndex
					for (size_t pii = primitiveCounter; pii < node.primitiveCount; ++pii) {
						const Primitive& primitive = model.primitives[model.primitiveIndices[pii]];
						if (ImGui::TreeNode(("Primitive " + std::to_string(pii)).c_str())) {

							if (primitive.material >= 0) {
								ImGui::Text(("Material: " + std::to_string(primitive.material)).c_str());
							}
							else {
								// Default
							}
							ImGui::Text(("VAO: " + std::to_string(primitive.vao)).c_str());
							ImGui::Text(("Mode: " + std::to_string(primitive.mode)).c_str());
							ImGui::Text(("Count: " + std::to_string(primitive.count)).c_str());
							ImGui::Text(("EBO: " + std::to_string(primitive.ebo)).c_str());
							ImGui::Text(("IndexType: " + std::to_string(primitive.indexType)).c_str());

							ImGui::TreePop();
						}
					}

					primitiveCounter += node.primitiveCount;
					
					ImGui::TreePop();
				}
			}
		}
		ImGui::End();
		return open;
	}

}
