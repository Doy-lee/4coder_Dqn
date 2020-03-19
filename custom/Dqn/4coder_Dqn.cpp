/*
4coder_default_bidings.cpp - Supplies the default bindings used for default 4coder behavior.
*/

// TOP

#if !defined(FCODER_DEFAULT_BINDINGS_CPP)
#define FCODER_DEFAULT_BINDINGS_CPP

#include "4coder_default_include.cpp"

// NOTE(allen): Users can declare their own managed IDs here.
CUSTOM_ID(command_map, Dqn4Coder_MappingID_VimNormalMode);
i64 Dqn4Coder_MappingID_VimEditMode = 0;

#include "generated/managed_id_metadata.cpp"

#define FILE_SCOPE static
#define LOCAL_PERSIST static
#define CAST(x) (x)

struct VimCore
{
    b32 normal_mode;
    struct
    {
        b32 shift_modifier;
    } f_chord;
};

struct Core
{
    u64           curr_mapping_id;
    VimCore       vim;
    History_Group history_group;
    b32           history_group_started;
};
FILE_SCOPE Core core;

FILE_SCOPE void Dqn4Coder_SetCursorColour(Application_Links *app, int colour)
{
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

    core.curr_mapping_id = map_id;
    core.vim.normal_mode = map_id == CAST(Command_Map_ID) Dqn4Coder_MappingID_VimNormalMode;

    if (core.vim.normal_mode)
    {
        if (core.history_group_started)
        {
            history_group_end(core.history_group);
            core.history_group_started = false;
        }

        Dqn4Coder_SetCursorColour(app, 0xFFFF0000 /*ARGB*/);
    }
    else
    {
        if (!core.history_group_started)
        {
            core.history_group_started = true;
            core.history_group         = history_group_begin(app, *buffer_id);
        }
        Dqn4Coder_SetCursorColour(app, 0xFF00FF00 /*ARGB*/);
    }
}

CUSTOM_COMMAND_SIG(Dqn4Vim_NormalModeMappings)
CUSTOM_DOC("Use Vim normal mode key-bindings")
{
    Dqn4Coder_SetCurrentMapping(app, Dqn4Coder_MappingID_VimNormalMode, nullptr /*buffer_id*/);
}

CUSTOM_COMMAND_SIG(Dqn4Vim_EditModeMappings)
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

CUSTOM_COMMAND_SIG(Dqn4Vim_NewLineThenEditMode)
CUSTOM_DOC("Add new line and enter insert mode.")
{
    seek_pos_of_visual_line(app, Side_Max);
    write_string(app, string_u8_litexpr("\n"));
    Dqn4Coder_SetCurrentMapping(app, Dqn4Coder_MappingID_VimEditMode, nullptr);
}

CUSTOM_COMMAND_SIG(Dqn4Vim_PrependNewLineThenEditMode)
CUSTOM_DOC("Add new line and enter insert mode.")
{
    seek_pos_of_visual_line(app, Side_Min);
    write_string(app, string_u8_litexpr("\n"));
    View_ID view = get_active_view(app, Access_ReadVisible);
    view_set_cursor_by_character_delta(app, view, -1);
    Dqn4Coder_SetCurrentMapping(app, Dqn4Coder_MappingID_VimEditMode, nullptr);
}

CUSTOM_COMMAND_SIG(Dqn4Vim_AppendOneAfterCursor)
CUSTOM_DOC("Enter insert mode one character after the cursor.")
{
    // TODO(doyle): Vim behaviour is to return the cursor 1 character back after
    // insertion. Test what feels better.
    View_ID view = get_active_view(app, Access_ReadVisible);
    view_set_cursor_by_character_delta(app, view, 1);
    no_mark_snap_to_cursor_if_shift(app, view);
    Dqn4Coder_SetCurrentMapping(app, Dqn4Coder_MappingID_VimEditMode, nullptr);
}

CUSTOM_COMMAND_SIG(Dqn4Vim_ReplaceAtCursorThenEditMode)
CUSTOM_DOC("Delete the character at the cursor then enter edit mode.")
{
    delete_char(app);
    View_ID view = get_active_view(app, Access_ReadVisible);
    no_mark_snap_to_cursor_if_shift(app, view);
    Dqn4Coder_SetCurrentMapping(app, Dqn4Coder_MappingID_VimEditMode, nullptr);
}

CUSTOM_COMMAND_SIG(Dqn4Vim_DeleteToNextToken)
CUSTOM_DOC("Delete from the current cursor to the next token.")
{
    // TODO(doyle): Slightly wrong, MoveToNextToken() ignores comments
    View_ID view     = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);

    i64 start_cursor_p = view_get_cursor_pos(app, view);
    Dqn4Vim_MoveToNextToken(app);
    i64 end_cursor_p = view_get_cursor_pos(app, view);

    Range_i64 range     = {};
    range.first         = start_cursor_p;
    range.one_past_last = end_cursor_p;
    buffer_replace_range(app, buffer, range, string_u8_empty);
}

// -------------------------------------------------------------------------------------------------
//
// Dqn4Vim: Select Chord Mappings
//
// -------------------------------------------------------------------------------------------------
CUSTOM_COMMAND_SIG(Dqn4Vim_ChordF)
{
    Assert(core.curr_mapping_id == Dqn4Coder_MappingID_VimNormalMode);

    Scratch_Block scratch(app);
    Input_Modifier_Set mods = system_get_keyboard_modifiers(scratch);

    User_Input user_input = get_next_input(app, EventProperty_TextInsert, EventProperty_Escape);
    if (user_input.abort)
        return;

    String_Const_u8 input = to_writable(&user_input);
    View_ID view          = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer      = view_get_buffer(app, view, Access_ReadWriteVisible);

    i64 cursor_p = view_get_cursor_pos(app, view);
    i64 end      = 0;
    i64 match_p  = 0;
    if (has_modifier(&mods, KeyCode_Shift)) // Search backwards
    {
        end = -1;
        seek_string_backward(app, buffer, cursor_p, end, input, &match_p);
    }
    else
    {
        end = buffer_get_size(app, buffer);
        seek_string_forward(app, buffer, cursor_p, end, input, &match_p);
    }

    if (match_p != end)
        view_set_cursor(app, view, seek_pos(match_p));
}

FILE_SCOPE void Dqn4Vim__ChordCOrD(Application_Links *app, b32 c_chord)
{
    Assert(core.curr_mapping_id == Dqn4Coder_MappingID_VimNormalMode);
    // NOTE: We activate Edit Mode mappings early to start recording the edit history into one batch.
    if (c_chord)
        Dqn4Vim_EditModeMappings(app);

    b32 handled = true;
    User_Input user_input = get_next_input(app, EventProperty_TextInsert, EventProperty_Escape);
    if (user_input.abort)
    {
        handled = false;
    }
    else
    {
        String_Const_u8 input = to_writable(&user_input);
        View_ID view          = get_active_view(app, Access_ReadVisible);
        Buffer_ID buffer      = view_get_buffer(app, view, Access_ReadWriteVisible);
        char ch               = character_to_lower(input.str[0]);

        if (ch == 'i' || ch == 't')
        {
            User_Input next_user_input = get_next_input(app, EventProperty_TextInsert, EventProperty_Escape);
            if (next_user_input.abort)
            {
                handled = false;
            }
            else
            {
                String_Const_u8 next_input = to_writable(&next_user_input);
                char next_ch               = next_input.str[0];
                switch(ch)
                {
                    case 't':
                    {
                        i64 start_pos = view_get_cursor_pos(app, view);
                        i64 find_pos  = 0;
                        seek_string_forward(app, buffer, start_pos, 0 /*end*/, next_input, &find_pos);

                        Range_i64 range     = {};
                        range.first         = start_pos;
                        range.one_past_last = find_pos;
                        buffer_replace_range(app, buffer, range, string_u8_empty);
                    }
                    break;

                    case 'i':
                    {
                        if (next_ch == '(' || next_ch == ')' ||
                            next_ch == '{' || next_ch == '}' ||
                            next_ch == '<' || next_ch == '>' ||
                            next_ch == '[' || next_ch == ']' ||
                            next_ch == '"' || next_ch == '\''
                           )
                        {
                            i64 cursor_p = view_get_cursor_pos(app, view);
                            i64 end      = buffer_get_size(app, buffer);

                            char search_pair[2] = {};
                            switch(next_ch)
                            {
                                case '(': case ')': search_pair[0] = '('; search_pair[1] = ')'; break;
                                case '{': case '}': search_pair[0] = '{'; search_pair[1] = '}'; break;
                                case '<': case '>': search_pair[0] = '<'; search_pair[1] = '>'; break;
                                case '[': case ']': search_pair[0] = '['; search_pair[1] = ']'; break;
                                case '"': case '\'': search_pair[0] = next_ch;  search_pair[1] = next_ch; break;
                                default: InvalidPath;
                            }

                            i64 start_p = 0, end_p = 0;
                            seek_string_backward(app, buffer, cursor_p, 0 /*min*/, SCu8(&search_pair[0], 1), &start_p);
                            seek_string_forward(app, buffer, cursor_p, end, SCu8(&search_pair[1], 1), &end_p);

                            if (start_p == -1 || end_p == end)
                            {
                                // NOTE: Failed to find enclosing pairs, just do nothing
                                handled = false;
                            }
                            else
                            {
                                Range_i64 range = {start_p + 1, end_p};
                                buffer_replace_range(app, buffer, range, string_u8_empty);
                            }
                        }
                    }
                }
            }
        }
        else if (ch == 'w')
        {
            Dqn4Vim_DeleteToNextToken(app);
        }
        else
        {
            InvalidPath;
        }
    }

    if (!handled && c_chord)
        Dqn4Vim_NormalModeMappings(app);
}

CUSTOM_COMMAND_SIG(Dqn4Vim_ChordC)
{
    Dqn4Vim__ChordCOrD(app, true /*c_chord*/);
}

CUSTOM_COMMAND_SIG(Dqn4Vim_ChordD)
{
    Dqn4Vim__ChordCOrD(app, false /*c_chord*/);
}

FILE_SCOPE void Dqn4Coder_SetDefaultMappings(Mapping *mapping)
{
    Dqn4Coder_MappingID_VimEditMode = mapid_code;
    setup_default_mapping(mapping, mapid_global, mapid_file, mapid_code);

    MappingScope();
    SelectMapping(mapping);

    SelectMap(Dqn4Coder_MappingID_VimNormalMode);
    {
        ParentMap(mapid_global);
        BindMouse       (click_set_cursor_and_mark, MouseCode_Left);
        BindMouseRelease(click_set_cursor, MouseCode_Left);
        BindCore        (click_set_cursor_and_mark, CoreCode_ClickActivateView);
        BindMouseMove   (click_set_cursor_if_lbutton);

        Bind(move_up,    KeyCode_K);
        Bind(move_down,  KeyCode_J);
        Bind(move_left,  KeyCode_H);
        Bind(move_right, KeyCode_L);
        Bind(page_up,    KeyCode_U, KeyCode_Control);
        Bind(page_down,  KeyCode_D, KeyCode_Control);

        Bind(redo,              KeyCode_R);
        Bind(undo,              KeyCode_U);
        Bind(search,            KeyCode_ForwardSlash);
        Bind(search_identifier, KeyCode_ForwardSlash, KeyCode_Control);

        Bind(delete_char,             KeyCode_X);
        Bind(copy,                    KeyCode_Y);
        Bind(paste,                   KeyCode_P);
        Bind(set_mark,                KeyCode_Space);
        Bind(save,                    KeyCode_S, KeyCode_Control);
        Bind(interactive_open_or_new, KeyCode_O, KeyCode_Control);

        Bind(Dqn4Vim_NewLineThenEditMode,         KeyCode_O);
        Bind(Dqn4Vim_PrependNewLineThenEditMode,  KeyCode_O, KeyCode_Shift);
        Bind(Dqn4Vim_EditModeMappings,            KeyCode_I);
        Bind(Dqn4Vim_AppendOneAfterCursor,        KeyCode_A);
        Bind(Dqn4Vim_ReplaceAtCursorThenEditMode, KeyCode_S);

        Bind(Dqn4Vim_MoveToNextToken,     KeyCode_W);
        Bind(Dqn4Vim_MoveToPreviousToken, KeyCode_B);
        Bind(Dqn4Vim_MoveToStartOfLine,   KeyCode_6, KeyCode_Shift);
        Bind(Dqn4Vim_MoveToEndOfLine,     KeyCode_4, KeyCode_Shift);

        // NOTE: Chords
        Bind(Dqn4Vim_ChordC, KeyCode_C);
        Bind(Dqn4Vim_ChordD, KeyCode_D);
        Bind(Dqn4Vim_ChordF, KeyCode_F);
        Bind(Dqn4Vim_ChordF, KeyCode_F, KeyCode_Shift);

        // TODO(doyle): Changing panels should really be moving to the split in
        // the direction of the home movement keys. Not just cycling through all
        // panels
        Bind(change_active_panel,                           KeyCode_L, KeyCode_Control);
        Bind(change_active_panel_backwards,                 KeyCode_H, KeyCode_Control);
        Bind(interactive_kill_buffer,                       KeyCode_K, KeyCode_Control);
        Bind(interactive_switch_buffer,                     KeyCode_I, KeyCode_Control);
        Bind(command_lister,                                KeyCode_X, KeyCode_Alt);
        Bind(list_all_locations_of_identifier,              KeyCode_T, KeyCode_Control, KeyCode_Shift);
        Bind(list_all_locations,                            KeyCode_F, KeyCode_Control, KeyCode_Shift);
        Bind(list_all_substring_locations_case_insensitive, KeyCode_F, KeyCode_Alt);
    }

    SelectMap(Dqn4Coder_MappingID_VimEditMode);
    {
        ParentMap(mapid_global);
        Bind(Dqn4Vim_NormalModeMappings, KeyCode_Escape);
        Bind(Dqn4Vim_NormalModeMappings, KeyCode_CapsLock);
        Bind(backspace_char,             KeyCode_Backspace);

        // NOTE: Copied from setup_default_mapping
        BindTextInput(write_text_and_auto_indent);
        Bind(move_left_alpha_numeric_boundary,           KeyCode_Left, KeyCode_Control);
        Bind(move_right_alpha_numeric_boundary,          KeyCode_Right, KeyCode_Control);
        Bind(move_left_alpha_numeric_or_camel_boundary,  KeyCode_Left, KeyCode_Alt);
        Bind(move_right_alpha_numeric_or_camel_boundary, KeyCode_Right, KeyCode_Alt);
        Bind(comment_line_toggle,        KeyCode_Semicolon, KeyCode_Control);
        Bind(word_complete,              KeyCode_Tab);
        Bind(auto_indent_range,          KeyCode_Tab, KeyCode_Control);
        Bind(auto_indent_line_at_cursor, KeyCode_Tab, KeyCode_Shift);
        Bind(word_complete_drop_down,    KeyCode_Tab, KeyCode_Shift, KeyCode_Control);
        Bind(write_block,                KeyCode_R, KeyCode_Alt);
        Bind(write_todo,                 KeyCode_T, KeyCode_Alt);
        Bind(write_note,                 KeyCode_Y, KeyCode_Alt);
        Bind(list_all_locations_of_type_definition,               KeyCode_D, KeyCode_Alt);
        Bind(list_all_locations_of_type_definition_of_identifier, KeyCode_T, KeyCode_Alt, KeyCode_Shift);
        Bind(open_long_braces,           KeyCode_LeftBracket, KeyCode_Control);
        Bind(open_long_braces_semicolon, KeyCode_LeftBracket, KeyCode_Control, KeyCode_Shift);
        Bind(open_long_braces_break,     KeyCode_RightBracket, KeyCode_Control, KeyCode_Shift);
        Bind(select_surrounding_scope,   KeyCode_LeftBracket, KeyCode_Alt);
        Bind(select_surrounding_scope_maximal, KeyCode_LeftBracket, KeyCode_Alt, KeyCode_Shift);
        Bind(select_prev_scope_absolute, KeyCode_RightBracket, KeyCode_Alt);
        Bind(select_prev_top_most_scope, KeyCode_RightBracket, KeyCode_Alt, KeyCode_Shift);
        Bind(select_next_scope_absolute, KeyCode_Quote, KeyCode_Alt);
        Bind(select_next_scope_after_current, KeyCode_Quote, KeyCode_Alt, KeyCode_Shift);
        Bind(place_in_scope,             KeyCode_ForwardSlash, KeyCode_Alt);
        Bind(delete_current_scope,       KeyCode_Minus, KeyCode_Alt);
        Bind(if0_off,                    KeyCode_I, KeyCode_Alt);
        Bind(open_file_in_quotes,        KeyCode_1, KeyCode_Alt);
        Bind(open_matching_file_cpp,     KeyCode_2, KeyCode_Alt);
        Bind(write_zero_struct,          KeyCode_0, KeyCode_Control);
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
    push_fancy_string(scratch, &list, base_color, core.vim.normal_mode ? string_u8_litexpr("NORMAL Mode") : string_u8_litexpr("EDIT Mode"));

    Vec2_f32 p = bar.p0 + V2f32(2.f, 2.f);
    draw_fancy_line(app, face_id, fcolor_zero(), &list, p);
}

void Dqn4Coder_DefaultRenderCaller(Application_Links *app, Frame_Info frame_info, View_ID view_id){
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
BUFFER_HOOK_SIG(Dqn4Coder_DefaultBeginBuffer)
{
    default_begin_buffer(app, buffer_id);
    Dqn4Coder_SetCurrentMapping(app, Dqn4Coder_MappingID_VimNormalMode, &buffer_id);
    return (0); // No meaning for return
}

void Dqn4Coder_HookDefaults(Application_Links *app)
{
    set_custom_hook(app, HookID_BeginBuffer, Dqn4Coder_DefaultBeginBuffer);
    set_custom_hook(app, HookID_RenderCaller, Dqn4Coder_DefaultRenderCaller);
}

void custom_layer_init(Application_Links *app){
    Thread_Context *tctx = get_thread_context(app);

    // NOTE(allen): setup for default framework
    default_framework_init(app);

    // NOTE(allen): default hooks and command maps
    set_all_default_hooks(app);
    mapping_init(tctx, &framework_mapping);
    Dqn4Coder_SetDefaultMappings(&framework_mapping);
    Dqn4Coder_HookDefaults(app);
}

#endif //FCODER_DEFAULT_BINDINGS

// BOTTOM

