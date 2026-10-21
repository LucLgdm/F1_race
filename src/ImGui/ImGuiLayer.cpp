/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ImGuiLayer.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lde-merc <lde-merc@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/04 12:56:58 by lde-merc          #+#    #+#             */
/*   Updated: 2026/06/23 15:59:42 by lde-merc         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ImGui/ImGuiLayer.hpp"

constexpr float panelWidth = 350.0f;

ImGuiLayer::~ImGuiLayer() {
	shutdown();
}

/************************************************************************
 * Necessary
 * **********************************************************************/
void ImGuiLayer::init(GLFWwindow* window, int width, int height) {
	_openglwidth = (float)width;
	_openglheight = (float)height;
	_viewPortWidth = _openglwidth - panelWidth;
	_viewPortHeight = _openglheight;
	
	if (_initialized)
		return;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui::StyleColorsDark();
	ImGui_ImplOpenGL3_Init("#version 330");
	
	ImGuiStyle* style = &ImGui::GetStyle();
	style->GrabRounding = 4.0f;
	ImVec4* colors = style->Colors;
	// colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::GRAY100);
	// colors[ImGuiCol_ScrollbarBg] = ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::GRAY100); // same as regular background
	colors[ImGuiCol_ScrollbarGrab] = ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::Static::GRAY400);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::Static::GRAY600);
	colors[ImGuiCol_ScrollbarGrabActive] = ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::Static::GRAY700);
	colors[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::Static::BLUE500);
	colors[ImGuiCol_SliderGrab] = ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::Static::GRAY700);
	colors[ImGuiCol_SliderGrabActive] = ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::Static::GRAY800);
	colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::Static::BLUE400);
	colors[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::Static::BLUE500);
	colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::Static::BLUE600);

	style->WindowRounding = 5.0f;
	style->WindowBorderSize = 2.0f;
	style->FrameBorderSize = 1.0f;

	colors[ImGuiCol_Border] = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
	style->FrameRounding = 5.0f;
	
	_initialized = true;
	std::cout << "\033[32m[ImGuiLayer]\033[0m \033[33mImGui layer initialized.\033[0m" << std::endl;
}

void ImGuiLayer::beginFrame() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	// ImGuizmo::BeginFrame();
}

void ImGuiLayer::endFrame() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::shutdown() {
	if (!_initialized)
		return;
	
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	_initialized = false;
}

/************************************************************************
 * Render
 * **********************************************************************/
void ImGuiLayer::render(TrainingSettings& settings, const TrainingStats& stats) {
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(panelWidth, panelWidth));
	ImGui::Begin("Entrainement");

	ImGui::Checkbox("Pause", &settings.paused);
	ImGui::Checkbox("Afficher la simulation", &settings.showSimulation);
	ImGui::Checkbox("Entrainement accelere", &settings.accelerated);

	ImGui::Separator();
	ImGui::Text("Generation : %llu", static_cast<unsigned long long> (stats.generation));
	ImGui::Text("Agents actifs : %llu / %llu",
				static_cast<unsigned long long> (stats.activeAgents),
				static_cast<unsigned long long> (stats.populationSize));
	
	if (stats.hasChampion)
		ImGui::Text("Record : %.3f s", stats.bestLapTime);
	else
		ImGui::TextUnformatted("Aucun champion");
	
	ImGui::Separator();
	ImGui::InputText("Fichier Champion", _championPath, sizeof(_championPath));
	if (ImGui::Button("Reprendre l4entrainement")) {
		_loadRequest = ChampionLoadRequest{_championPath, ChampionLoadMode::ResumeTraining};
	}

	if (ImGui::Button("Voir le champion"))
		_loadRequest = ChampionLoadRequest{_championPath, ChampionLoadMode::Replay};

	if (!_errorMessage.empty())
		ImGui::TextWrapped("%s", _errorMessage.c_str());

	ImGui::End();
}






/*******************************
 * Tools
 ******************************/
std::optional<ChampionLoadRequest> ImGuiLayer::takeLoadRequest() {
	return std::exchange(_loadRequest, std::nullopt);
}

void ImGuiLayer::setError(std::string message) {
	_errorMessage = message;
}
