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
#include <cstdio>

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
	// std::cout << "\033[32m[ImGuiLayer]\033[0m \033[33mImGui layer initialized.\033[0m" << std::endl;
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
	ImGui::SetNextWindowSize(ImVec2(panelWidth, std::min(settings.fullRace ? 800.0f : 620.0f, ImGui::GetIO().DisplaySize.y)));
	ImGui::Begin("Entrainement");

	if (ImGui::Button("Editeur de circuit")) _editorRequest = true;
    int mode=settings.mode==DrivingMode::Chrono ? 0 : (settings.fullRace ? 2 : 1);
    if (ImGui::Combo("Mode", &mode, "Chrono\0Course - depassement\0Course complete\0")) {
        settings.mode=mode==0 ? DrivingMode::Chrono : DrivingMode::Race;
        settings.fullRace=mode==2;
        settings.paused=true;
    }
    if (settings.mode==DrivingMode::Race) {
        if (!settings.fullRace) ImGui::TextUnformatted("Prochaine course complete");
        ImGui::InputInt("Voitures", &settings.raceCars);
        ImGui::InputInt("Tours", &settings.raceLaps);
        settings.raceCars=std::clamp(settings.raceCars,2,20);
        settings.raceLaps=std::clamp(settings.raceLaps,1,20);
        if (settings.fullRace && ImGui::Button("Nouvelle course")) settings.restartRace=true;
        ImGui::TextWrapped("Reglages appliques au prochain depart.");
    }
    ImGui::Checkbox("Pause", &settings.paused);
	ImGui::Checkbox("Afficher la simulation", &settings.showSimulation);
	ImGui::Checkbox("Vue de tout le circuit", &settings.trackOverview);
	ImGui::Checkbox(settings.fullRace ? "Simulation acceleree" : "Entrainement accelere", &settings.accelerated);

	ImGui::Separator();
    if (settings.fullRace) {
        ImGui::Text("Course %s | %.2f s",stats.raceFinished ? "terminee" : "en cours",stats.raceElapsed);
        ImGui::Text("Objectif : %llu tours | contacts : %llu",static_cast<unsigned long long>(stats.raceLapTarget),
                    static_cast<unsigned long long>(stats.carContacts));
        if (ImGui::BeginTable("Classement",4,ImGuiTableFlags_RowBg|ImGuiTableFlags_ScrollY,ImVec2(0,180))) {
            ImGui::TableSetupColumn("Pos."); ImGui::TableSetupColumn("Voiture");
            ImGui::TableSetupColumn("Tours"); ImGui::TableSetupColumn("Etat / temps");
            ImGui::TableHeadersRow();
            for (std::size_t rank=0;rank<stats.standings.size();++rank) {
                const auto& row=stats.standings[rank]; const auto color=raceCarColor(row.id);
                ImGui::TableNextRow(); ImGui::TableNextColumn();
                ImGui::Text("%llu",static_cast<unsigned long long>(rank+1));
                ImGui::TableNextColumn();
                ImGui::TextColored(ImVec4(color[0],color[1],color[2],1),"#%llu",static_cast<unsigned long long>(row.id+1));
                ImGui::TableNextColumn(); ImGui::Text("%llu",static_cast<unsigned long long>(row.laps));
                ImGui::TableNextColumn();
                if (row.status==DriverStatus::Finished) ImGui::Text("%.2f s",row.finishTime);
                else ImGui::TextUnformatted(driverStatusText(row.status));
            }
            ImGui::EndTable();
        }
    } else ImGui::Text("Generation : %llu", static_cast<unsigned long long> (stats.generation));
	ImGui::Text("Agents actifs : %llu / %llu",
				static_cast<unsigned long long> (stats.activeAgents),
				static_cast<unsigned long long> (stats.populationSize));
	
    if (settings.mode==DrivingMode::Race && !settings.fullRace) {
        ImGui::Text("Meilleur bilan termine : %llu / 5", static_cast<unsigned long long>(stats.raceSuccesses));
        ImGui::Text("Contacts voitures : %llu | piste : %llu",
            static_cast<unsigned long long>(stats.carContacts),static_cast<unsigned long long>(stats.wallContacts));
        ImGui::Text("Essai affiche : %llu / 5",static_cast<unsigned long long>(stats.raceScenario+1));
        ImGui::TextWrapped("Chaque candidat roule dans son propre exercice face a une voiture lente.");
    }
    if (stats.hasChampion)
        ImGui::Text(settings.mode==DrivingMode::Race ? "Record exercices : %.3f s" : "Record : %.3f s", stats.bestLapTime);
	else
		ImGui::TextUnformatted(settings.fullRace ? "Reseau de travail non sauvegarde" : "Aucun champion");
	
	ImGui::Separator();
	ImGui::InputText("Fichier Champion", _championPath, sizeof(_championPath));
    if (!settings.fullRace) {
        if (ImGui::Button("Reprendre l entrainement"))
            _loadRequest=ChampionLoadRequest{_championPath,ChampionLoadMode::ResumeTraining};
        if (ImGui::Button("Voir le champion"))
            _loadRequest=ChampionLoadRequest{_championPath,ChampionLoadMode::Replay};
        if (ImGui::Button("Course depuis un champion chrono"))
            _loadRequest=ChampionLoadRequest{_championPath,ChampionLoadMode::SeedRace};
    }
    if (ImGui::Button(settings.fullRace ? "Charger ce reseau pour la course" : "Lancer une course avec ce reseau"))
        _loadRequest=ChampionLoadRequest{_championPath,ChampionLoadMode::StartFullRace};

    if (!_errorMessage.empty())
		ImGui::TextWrapped("%s", _errorMessage.c_str());

	_panelRight = ImGui::GetWindowPos().x + ImGui::GetWindowSize().x;
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

void ImGuiLayer::setChampionPath(const std::string& path) {
	std::snprintf(_championPath, sizeof(_championPath), "%s", path.c_str());
}
