#include <life_cycle.h>
#include <context.h>

#include <floral.h>
#include <stdio.h>
#include <clover.h>

#include <insigne/commons.h>
#include <insigne/memory.h>
#include <insigne/driver.h>
#include <insigne/render.h>
#include <insigne/renderer.h>
#include <insigne/buffers.h>

#include <imgui.h>

#include <platform/windows/event_defs.h>

/*
 * step 1: know your data
 * 	- Debug UI surface:
 * 		transparent + ldr
 * 		3 attribute locations are needed with format of pos.vec2 - uv.vec2 - vcolor.vec4
 * 	- Skybox surface:
 * 		solid + hdr
 * 		3 attribute locations needed with format of pos.vec3 - normal.vec3 - uv.vec2
 * 	- Solid PBR surface:
 * 		solid + pbr + hdr
 * 		3 attribute locations needed with format of pos.vec3 - normal.vec3 - uv.vec2
 * step 2: coding concept
 * 	- Declaration:
 * 		struct UISurface
 * 		draw_command_buffer_t<UISurface>
 * 		SetupDebugUIRenderState()				=> will be fed to the renderer
 * 		RenderDebugUI()							=> will be fed to the renderer
 *
 * 		struct SkyboxSurface
 * 		draw_command_buffer_t<SkyboxSurface>
 * 		SetupSkyboxRenderState()
 * 		RenderSkybox()
 *
 * 		struct SolidSurface
 * 		draw_command_buffer_t<SolidSurface>
 * 		SetupSolidRenderState()
 * 		RenderSolid()
 * 	- Insigne:
 * 		> General Command Buffer				=> handles: shader compiling, vbo, texture, ...
 * 		> Draw Command Buffer					=> handles: surface draw call
 * 	- Usage:
 * 		draw_surface<UISurface>()
 * 		draw_surface_segmented<UISurface>()
 */

struct ImGuiSurface : insigne::renderable_surface_t<ImGuiSurface> {
	static void setup_states()
	{
		using namespace insigne;
		// setup states
		renderer::set_blending<true_type>(blend_equation_e::func_add, factor_e::fact_src_alpha, factor_e::fact_one_minus_src_alpha);
		//renderer::set_cull_face<true_type>(front_face_e::face_ccw);
		renderer::set_depth_test<false_type>(compare_func_e::func_always);
		renderer::set_depth_write<false_type>();
	}
};

namespace insigne {
	template<>
	gpu_command_buffer_t draw_command_buffer_t<ImGuiSurface>::command_buffer[BUFFERED_FRAMES];
}

namespace calyx {

	void initialize()
	{
		printf("app init\n");
	}

	static insigne::surface_handle_t			s_ImGuiSurf;
	static insigne::texture_handle_t			s_ImGuiTex;
	static insigne::material_handle_t			s_ImGuiMaterial;
	static insigne::shader_handle_t				s_ImGuiShader;

	struct MouseState {
		f32										MouseX, MouseY;
		bool									MousePressed[2];
		bool									MouseHeldThisFrame[2];
	};

	static MouseState							s_MouseState;

	void RenderDrawLists(ImDrawData* drawData)
	{
		using namespace insigne;
		ImGuiIO& io = ImGui::GetIO();
		drawData->ScaleClipRects(ImVec2(1.0f, 1.0f));

#if 0
		set_blend(true);
		set_blend_equation(blend_equation_e::func_add);
		set_blend_function(factor_e::fact_src_alpha, factor_e::fact_one_minus_src_alpha);
		set_cull_face(false);
		set_depth_test(false);
		//set_scissor(true);
		for (s32 i = 0; i < drawData->CmdListsCount; i++) {
			const ImDrawList* cmdList = drawData->CmdLists[i];
			const ImDrawIdx* idxBufferOffset = 0;
			update_streamed_surface(s_ImGuiSurf,
					(voidptr)cmdList->VtxBuffer.Data, (size)cmdList->VtxBuffer.Size * sizeof(ImDrawVert),
					(voidptr)cmdList->IdxBuffer.Data, (size)cmdList->IdxBuffer.Size * sizeof(ImDrawIdx),
					cmdList->VtxBuffer.Size, cmdList->IdxBuffer.Size);
			for (s32 cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; cmdIdx++) {
				const ImDrawCmd* drawCmd = &cmdList->CmdBuffer[cmdIdx];
				if (drawCmd->UserCallback) {
					drawCmd->UserCallback(cmdList, drawCmd);
				} else {
					draw_surface_segmented(s_ImGuiSurf, s_ImGuiMaterial, (s32)drawCmd->ElemCount, (voidptr)idxBufferOffset);
				}
				idxBufferOffset += drawCmd->ElemCount;
			}
		}
#else
		for (s32 i = 0; i < drawData->CmdListsCount; i++) {
			const ImDrawList* cmdList = drawData->CmdLists[i];
			const ImDrawIdx* idxBufferOffset = 0;
			update_streamed_surface(s_ImGuiSurf,
					(voidptr)cmdList->VtxBuffer.Data, (size)cmdList->VtxBuffer.Size * sizeof(ImDrawVert),
					(voidptr)cmdList->IdxBuffer.Data, (size)cmdList->IdxBuffer.Size * sizeof(ImDrawIdx),
					cmdList->VtxBuffer.Size, cmdList->IdxBuffer.Size);
			for (s32 cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; cmdIdx++) {
				const ImDrawCmd* drawCmd = &cmdList->CmdBuffer[cmdIdx];
				if (drawCmd->UserCallback) {
					drawCmd->UserCallback(cmdList, drawCmd);
				} else {
					draw_surface_segmented<ImGuiSurface>(s_ImGuiSurf, s_ImGuiMaterial, (s32)drawCmd->ElemCount, (voidptr)idxBufferOffset);
				}
				idxBufferOffset += drawCmd->ElemCount;
			}
		}
#endif
	}


	struct SolidSurface {
		static void setup_states_and_render()
		{
			CLOVER_VERBOSE("setup_and_render_solid");
		}

		static void init_buffer(insigne::linear_allocator_t* i_allocator)
		{
			CLOVER_VERBOSE("init_solid_buffer");
		}
	};

	void run(event_buffer_t* i_evtBuffer)
	{
		using namespace insigne;
		printf("app run\n");
		initialize_driver();
		typedef type_list_1(ImGuiSurface)		SurfaceTypeList;
		initialize_render_thread<SurfaceTypeList>();
		wait_for_initialization();

		set_clear_color(0.3f, 0.4f, 0.5f, 1.0f);

		const_cstr vert =
			"#version 300 es\n"
			"layout (location = 0) in highp vec2 l_Position_L;\n"
			"layout (location = 1) in mediump vec2 l_TexCoord;\n"
			"layout (location = 2) in mediump vec4 l_VertColor;\n"
			"uniform highp mat4 u_DebugOrtho;\n"
			"out mediump vec2 o_TexCoord;\n"
			"out mediump vec4 o_VertColor;\n"
			"void main() {\n"
				"o_TexCoord = l_TexCoord;\n"
				"o_VertColor = l_VertColor;\n"
				"gl_Position = u_DebugOrtho * vec4(l_Position_L.x, l_Position_L.y, 0.0f, 1.0f);\n"
			"}\n";
		const_cstr frag =
			"#version 300 es\n"
			"layout (location = 0) out mediump vec4 o_Color;\n"
			"uniform mediump sampler2D u_Tex;\n"
			"in mediump vec2 o_TexCoord;\n"
			"in mediump vec4 o_VertColor;\n"
			"void main()\n"
			"{\n"
				"o_Color = o_VertColor * texture(u_Tex, o_TexCoord.st);\n"
			"}\n";

		ImGuiIO& io = ImGui::GetIO();
		u8* pixels;
		s32 width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		shader_param_list_t* plist = allocate_shader_param_list(4);
		plist->push_back(shader_param_t("u_Tex", param_data_type_e::param_sampler2d));
		plist->push_back(shader_param_t("u_DebugOrtho", param_data_type_e::param_mat4));
		
		s_ImGuiShader = compile_shader(vert, frag, plist);
		s_ImGuiSurf = create_streamed_surface(sizeof(ImDrawVert));
		s_ImGuiTex = upload_texture2d(width, height, texture_format_e::rgba, pixels);

		io.Fonts->TexID = (voidptr)(size)s_ImGuiTex;
		io.RenderDrawListsFn = &RenderDrawLists;

		s_ImGuiMaterial = create_material(s_ImGuiShader);
		param_id tid = get_material_param<texture_handle_t>(s_ImGuiMaterial, "u_Tex");
		set_material_param(s_ImGuiMaterial, tid, s_ImGuiTex);

		io.DisplaySize = ImVec2((f32)g_context_attribs->window_width, (f32)g_context_attribs->window_height);
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
		floral::mat4x4f debugOrtho(
				floral::vec4f(2.0f / io.DisplaySize.x, 0.0f, 0.0f, 0.0f),
				floral::vec4f(0.0f, -2.0f / io.DisplaySize.y, 0.0f, 0.0f),
				floral::vec4f(0.0f, 0.0f, -1.0f, 0.0f),
				floral::vec4f(-1.0f, 1.0f, 0.0f, 1.0f));

		param_id mid = get_material_param<floral::mat4x4f>(s_ImGuiMaterial, "u_DebugOrtho");
		set_material_param(s_ImGuiMaterial, mid, debugOrtho);

		calyx::interact_event_t event;
		u32 f = 0;
		while (true) {
			f++;
			while (i_evtBuffer->try_pop_into(event)) {
				switch (event.event_type) {
					case calyx::interact_event_e::cursor_interact:
						{
							if (TEST_BIT(event.payload, CLX_MOUSE_LEFT_BUTTON)) {
								s_MouseState.MousePressed[0] = TEST_BIT_BOOL(event.payload, CLX_MOUSE_BUTTON_PRESSED);
								if (TEST_BIT_BOOL(event.payload, CLX_MOUSE_BUTTON_PRESSED))
									s_MouseState.MouseHeldThisFrame[0] = true;
							}
							if (TEST_BIT(event.payload, CLX_MOUSE_RIGHT_BUTTON)) {
								s_MouseState.MousePressed[1] = TEST_BIT_BOOL(event.payload, CLX_MOUSE_BUTTON_PRESSED);
								if (TEST_BIT_BOOL(event.payload, CLX_MOUSE_BUTTON_PRESSED))
									s_MouseState.MouseHeldThisFrame[1] = true;
							}
							break;
						}

					case calyx::interact_event_e::cursor_move:
						{
							u32 x = event.payload & 0xFFFF;
							u32 y = (event.payload & 0xFFFF0000) >> 16;
							s_MouseState.MouseX = (f32)x;
							s_MouseState.MouseY = (f32)y;
							break;
						}

					case calyx::interact_event_e::scroll:
						{
							break;
						}

					case calyx::interact_event_e::character_input:
						{
							break;
						}

					default:
						break;
				};
			}
			insigne::begin_frame();
			// imgui update
			io.MousePos = ImVec2(s_MouseState.MouseX, s_MouseState.MouseY);
			for (s32 i = 0; i < 2; i++) {
				io.MouseDown[i] = s_MouseState.MouseHeldThisFrame[i] || s_MouseState.MousePressed[i];
				s_MouseState.MouseHeldThisFrame[i] = false;
			}
			io.DeltaTime = 1.0f / 60.0f;

			ImGui::NewFrame();
			ImGui::ShowTestWindow();
			ImGui::Render();
			insigne::end_frame();
			insigne::dispatch_frame();
		}
	}

	void clean_up()
	{
		ImGui::Shutdown();
		printf("app cleanup\n");
	}

}
