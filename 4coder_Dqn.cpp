/*
4coder_default_bidings.cpp - Supplies the default bindings used for default 4coder behavior.
*/

// TOP

#if !defined(FCODER_DEFAULT_BINDINGS_CPP)
#define FCODER_DEFAULT_BINDINGS_CPP

#include "4coder_default_include.cpp"

// NOTE(allen): Users can declare their own managed IDs here.
CUSTOM_ID(command_map, Dqn4Coder_MappingID_VimNormalMode);
CUSTOM_ID(command_map, Dqn4Coder_MappingID_VimEditMode);

#include "generated/managed_id_metadata.cpp"

#define FILE_SCOPE static

struct Dqn4State
{
    b32 normal_mode;
};
FILE_SCOPE Dqn4State dqn_state;

FILE_SCOPE void
Dqn4Coder_SetCursorColour(Application_Links* app, int colour) {
    Color_Table *table             = &active_color_table;
    Arena *arena                   = &global_theme_arena;
    table->arrays[defcolor_cursor] = make_colors(arena, colour);
    table->arrays[defcolor_mark]   = make_colors(arena, colour);
}

FILE_SCOPE void Dqn4Coder_SetCurrentMapping(Application_Links *app, Command_Map_ID map_id, Buffer_ID *buffer_id)
{
    Buffer_ID buffer_id_ = {};
    if (!buffer_id)
    {
        buffer_id  = &buffer_id_;
        *buffer_id = view_get_buffer(app, get_active_view(app, Access_ReadVisible), Access_ReadVisible);
    }

    Managed_Scope scope        = buffer_get_managed_scope(app, *buffer_id);
    Command_Map_ID *map_id_ptr = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
    *map_id_ptr                = map_id;

    if (map_id == (Command_Map_ID)Dqn4Coder_MappingID_VimNormalMode)
    {
        dqn_state.normal_mode = true;
        Dqn4Coder_SetCursorColour(app, 0xFFFF0000 /*ARGB*/);
    }
    else if (map_id == (Command_Map_ID)Dqn4Coder_MappingID_VimEditMode)
    {
        dqn_state.normal_mode = false;
        Dqn4Coder_SetCursorColour(app, 0xFF00FF00 /*ARGB*/);
    }
}

CUSTOM_COMMAND_SIG(Dqn4Vim_UseNormalModeBindings)
CUSTOM_DOC("Use Vim normal mode key-bindings")
{
    Dqn4Coder_SetCurrentMapping(app, Dqn4Coder_MappingID_VimNormalMode, nullptr /*buffer_id*/);
}

CUSTOM_COMMAND_SIG(Dqn4Vim_UseEditModeBindings)
CUSTOM_DOC("Use Vim edit mode key-bindings")
{
    Dqn4Coder_SetCurrentMapping(app, Dqn4Coder_MappingID_VimEditMode, nullptr /*buffer_id*/);
}

CUSTOM_COMMAND_SIG(Dqn4Vim_MoveToPreviousToken)
CUSTOM_DOC("Seek left to the next closest token.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Backward, push_boundary_list(scratch, boundary_token));
}


CUSTOM_COMMAND_SIG(Dqn4Vim_MoveToNextToken)
CUSTOM_DOC("Seek right to the next closest token.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Forward, push_boundary_list(scratch, boundary_token));
}

CUSTOM_COMMAND_SIG(Dqn4Vim_MoveToStartOfLine)
CUSTOM_DOC("Seek to the start of the line.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Backward, push_boundary_list(scratch, boundary_line));
}

CUSTOM_COMMAND_SIG(Dqn4Vim_MoveToEndOfLine)
CUSTOM_DOC("Seek to the end of the line.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Forward, push_boundary_list(scratch, boundary_line));
}

FILE_SCOPE void Dqn4Coder_SetDefaultMappings(Mapping *mapping)
{
    setup_default_mapping(mapping, mapid_global, mapid_file, mapid_code);

    MappingScope();
    SelectMapping(mapping);
    SelectMap(Dqn4Coder_MappingID_VimNormalMode);
    {
        ParentMap(mapid_global);
        Bind(move_up, KeyCode_K);
        Bind(move_down, KeyCode_J);
        Bind(move_left, KeyCode_H);
        Bind(move_right, KeyCode_L);

        Bind(redo, KeyCode_R);
        Bind(undo, KeyCode_U);
        Bind(page_up, KeyCode_U, KeyCode_Control);
        Bind(page_down, KeyCode_D, KeyCode_Control);

        Bind(Dqn4Vim_UseEditModeBindings, KeyCode_I);
        Bind(Dqn4Vim_MoveToNextToken, KeyCode_W);
        Bind(Dqn4Vim_MoveToPreviousToken, KeyCode_B);

        Bind(Dqn4Vim_MoveToStartOfLine, KeyCode_6, KeyCode_Shift);
        Bind(Dqn4Vim_MoveToEndOfLine, KeyCode_4, KeyCode_Shift);
    }

    SelectMap(Dqn4Coder_MappingID_VimEditMode);
    {
        ParentMap(mapid_code);
        Bind(Dqn4Vim_UseNormalModeBindings, KeyCode_Escape);
        Bind(Dqn4Vim_UseNormalModeBindings, KeyCode_CapsLock);
    }
}

void Dqn4Coder_DrawFileBar(Application_Links *app, View_ID view_id, Buffer_ID buffer, Face_ID face_id, Rect_f32 bar)
{
    Scratch_Block scratch(app);
    draw_rectangle_fcolor(app, bar, 0.f, fcolor_id(defcolor_bar));

    FColor base_color = fcolor_id(defcolor_base);
    FColor pop2_color = fcolor_id(defcolor_pop2);

    i64 cursor_position = view_get_cursor_pos(app, view_id);
    Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(cursor_position));

    Fancy_Line list = {};
    String_Const_u8 unique_name = push_buffer_unique_name(app, scratch, buffer);
    push_fancy_string(scratch, &list, base_color, unique_name);
    push_fancy_stringf(scratch, &list, base_color, " - Row: %3.lld Col: %3.lld -", cursor.line, cursor.col);

    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Line_Ending_Kind *eol_setting = scope_attachment(app, scope, buffer_eol_setting,
                                                     Line_Ending_Kind);
    switch (*eol_setting){
        case LineEndingKind_Binary:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" bin"));
        }break;

        case LineEndingKind_LF:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" lf"));
        }break;

        case LineEndingKind_CRLF:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" crlf"));
        }break;
    }

    u8 space[3];
    {
        Dirty_State dirty = buffer_get_dirty_state(app, buffer);
        String_u8 str = Su8(space, 0, 3);
        if (dirty != 0){
            string_append(&str, string_u8_litexpr(" "));
        }
        if (HasFlag(dirty, DirtyState_UnsavedChanges)){
            string_append(&str, string_u8_litexpr("*"));
        }
        if (HasFlag(dirty, DirtyState_UnloadedChanges)){
            string_append(&str, string_u8_litexpr("!"));
        }
        push_fancy_string(scratch, &list, pop2_color, str.string);
    }

    push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" - "));
    push_fancy_string(scratch, &list, base_color, dqn_state.normal_mode ? string_u8_litexpr("NORMAL Mode") : string_u8_litexpr("EDIT Mode"));

    Vec2_f32 p = bar.p0 + V2f32(2.f, 2.f);
    draw_fancy_line(app, face_id, fcolor_zero(), &list, p);
}

void Dqn4Coder_HookRenderCall(Application_Links *app, Frame_Info frame_info, View_ID view_id){
    ProfileScope(app, __func__);
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);

    Rect_f32 region = draw_background_and_margin(app, view_id, is_active_view);
    Rect_f32 prev_clip = draw_set_clip(app, region);

    Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 line_height = face_metrics.line_height;
    f32 digit_advance = face_metrics.decimal_digit_advance;

    // NOTE(allen): file bar
    b64 showing_file_bar = false;
    if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) && showing_file_bar){
        Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
        Dqn4Coder_DrawFileBar(app, view_id, buffer, face_id, pair.min);
        region = pair.max;
    }
    
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);
    
    Buffer_Point_Delta_Result delta = delta_apply(app, view_id,
                                                  frame_info.animation_dt, scroll);
    if (!block_match_struct(&scroll.position, &delta.point)){
        block_copy_struct(&scroll.position, &delta.point);
        view_set_buffer_scroll(app, view_id, scroll, SetBufferScroll_NoCursorChange);
    }
    if (delta.still_animating){
        animate_in_n_milliseconds(app, 0);
    }
    
    // NOTE(allen): query bars
    {
        Query_Bar *space[32];
        Query_Bar_Ptr_Array query_bars = {};
        query_bars.ptrs = space;
        if (get_active_query_bars(app, view_id, ArrayCount(space), &query_bars)){
            for (i32 i = 0; i < query_bars.count; i += 1){
                Rect_f32_Pair pair = layout_query_bar_on_top(region, line_height, 1);
                draw_query_bar(app, query_bars.ptrs[i], face_id, pair.min);
                region = pair.max;
            }
        }
    }
    
    // NOTE(allen): FPS hud
    if (show_fps_hud){
        Rect_f32_Pair pair = layout_fps_hud_on_bottom(region, line_height);
        draw_fps_hud(app, frame_info, face_id, pair.max);
        region = pair.min;
        animate_in_n_milliseconds(app, 1000);
    }
    
    // NOTE(allen): layout line numbers
    Rect_f32 line_number_rect = {};
    if (global_config.show_line_number_margins){
        Rect_f32_Pair pair = layout_line_number_margin(app, buffer, region, digit_advance);
        line_number_rect = pair.min;
        region = pair.max;
    }
    
    // NOTE(allen): begin buffer render
    Buffer_Point buffer_point = scroll.position;
    Text_Layout_ID text_layout_id = text_layout_create(app, buffer, region, buffer_point);
    
    // NOTE(allen): draw line numbers
    if (global_config.show_line_number_margins){
        draw_line_number_margin(app, view_id, buffer, face_id, text_layout_id, line_number_rect);
    }
    
    // NOTE(allen): draw the buffer
    default_render_buffer(app, view_id, face_id, buffer, text_layout_id, region);
    
    text_layout_free(app, text_layout_id);
    draw_set_clip(app, prev_clip);
}

// -------------------------------------------------------------------------------------------------
//
// Dqn4Coder_Hook
//
// -------------------------------------------------------------------------------------------------
BUFFER_HOOK_SIG(Dqn4Coder_HookBufferBegin)
{
    default_begin_buffer(app, buffer_id);
    Dqn4Coder_SetCurrentMapping(app, Dqn4Coder_MappingID_VimNormalMode, &buffer_id);
    return (0); // No meaning for return
}

void Dqn4Coder_HookDefaults(Application_Links *app)
{
    set_custom_hook(app, HookID_BeginBuffer, Dqn4Coder_HookBufferBegin);
    set_custom_hook(app, HookID_RenderCaller, Dqn4Coder_HookRenderCall);
}

void custom_layer_init(Application_Links *app){
    Thread_Context *tctx = get_thread_context(app);

    // NOTE(allen): setup for default framework
    async_task_handler_init(app, &global_async_system);
    code_index_init();
    buffer_modified_set_init();
    Profile_Global_List *list = get_core_profile_list(app);
    ProfileThreadName(tctx, list, string_u8_litexpr("main"));
    initialize_managed_id_metadata(app);
    set_default_color_scheme(app);

    // NOTE(allen): default hooks and command maps
    set_all_default_hooks(app);
    mapping_init(tctx, &framework_mapping);
    Dqn4Coder_SetDefaultMappings(&framework_mapping);
    Dqn4Coder_HookDefaults(app);
}

#endif //FCODER_DEFAULT_BINDINGS

// BOTTOM

