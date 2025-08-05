#!/usr/bin/env python3
"""
LLDB Formatters for o2 Framework Types  
Based on Framework.natvis
"""

import lldb

def __lldb_init_module(debugger, internal_dict):
    """Called when module is imported by LLDB"""
    
    # UID formatter
    debugger.HandleCommand('type summary add -F o2_lldb_formatters.uid_summary "o2::UID"')
    
    # Vec2 formatter  
    debugger.HandleCommand('type summary add -F o2_lldb_formatters.vec2_summary -x "^o2::Vec2<.+>$"')
    
    # Rect formatter
    debugger.HandleCommand('type summary add -F o2_lldb_formatters.rect_summary -x "^o2::Rect<.+>$"')
    
    # Color4 formatter
    debugger.HandleCommand('type summary add -F o2_lldb_formatters.color4_summary "o2::Color4"')
    
    # Vertex formatter
    debugger.HandleCommand('type summary add -F o2_lldb_formatters.vertex_summary "o2::Vertex"')
    
    # Ref formatter
    debugger.HandleCommand('type summary add -F o2_lldb_formatters.ref_summary -x "^o2::Ref<.+>$"')
    
    # WeakRef formatter
    debugger.HandleCommand('type summary add -F o2_lldb_formatters.weakref_summary -x "^o2::WeakRef<.+>$"')
    
    # Transform formatter
    debugger.HandleCommand('type summary add -F o2_lldb_formatters.transform_summary "o2::Transform"')
    
    # TimeStamp formatter
    debugger.HandleCommand('type summary add -F o2_lldb_formatters.timestamp_summary "o2::TimeStamp"')
    
    # Widget formatter
    debugger.HandleCommand('type summary add -F o2_lldb_formatters.widget_summary "o2::Widget"')
    
    # Actor formatter 
    debugger.HandleCommand('type summary add -F o2_lldb_formatters.actor_summary "o2::Actor"')
    
    # Sprite formatter
    debugger.HandleCommand('type summary add -F o2_lldb_formatters.sprite_summary "o2::Sprite"')
    
    print("o2 Framework LLDB formatters loaded")


def get_child_value(valobj, name):
    """Helper to safely get child value"""
    try:
        child = valobj.GetChildMemberWithName(name)
        if child.IsValid():
            return child.GetValue()
        return "?"
    except:
        return "?"


def get_child_int(valobj, name):
    """Helper to safely get child integer value"""
    try:
        child = valobj.GetChildMemberWithName(name)
        if child.IsValid():
            return child.GetValueAsSigned()
        return 0
    except:
        return 0


def uid_summary(valobj, internal_dict):
    """Format o2::UID"""
    try:
        data = valobj.GetChildMemberWithName("data")
        if not data.IsValid():
            return "UID(?)"
            
        # Extract 4 32-bit values from 16 bytes
        bytes_data = []
        for i in range(16):
            byte_val = data.GetChildAtIndex(i).GetValueAsUnsigned() & 0xFF
            bytes_data.append(byte_val)
        
        part1 = bytes_data[0] | (bytes_data[1] << 8) | (bytes_data[2] << 16) | (bytes_data[3] << 24)
        part2 = bytes_data[4] | (bytes_data[5] << 8) | (bytes_data[6] << 16) | (bytes_data[7] << 24)
        part3 = bytes_data[8] | (bytes_data[9] << 8) | (bytes_data[10] << 16) | (bytes_data[11] << 24)
        part4 = bytes_data[12] | (bytes_data[13] << 8) | (bytes_data[14] << 16) | (bytes_data[15] << 24)
        
        return f"{part1:X} - {part2:X} - {part3:X} - {part4:X}"
    except:
        return "UID(?)"


def vec2_summary(valobj, internal_dict):
    """Format o2::Vec2<T>"""
    x = get_child_value(valobj, "x")
    y = get_child_value(valobj, "y")
    return f"{{ {x} {y} }}"


def rect_summary(valobj, internal_dict):
    """Format o2::Rect<T>"""
    left = get_child_value(valobj, "left")
    bottom = get_child_value(valobj, "bottom")
    right = get_child_value(valobj, "right")
    top = get_child_value(valobj, "top")
    return f"{{ (L {left} B {bottom})-(R {right} T {top}) }}"


def color4_summary(valobj, internal_dict):
    """Format o2::Color4"""
    r = get_child_value(valobj, "r")
    g = get_child_value(valobj, "g")
    b = get_child_value(valobj, "b")
    a = get_child_value(valobj, "a")
    return f"{{ {r} {g} {b} {a} }}"


def vertex_summary(valobj, internal_dict):
    """Format o2::Vertex"""
    x = get_child_value(valobj, "x")
    y = get_child_value(valobj, "y")
    z = get_child_value(valobj, "z")
    color = get_child_value(valobj, "color")
    tu = get_child_value(valobj, "tu")
    tv = get_child_value(valobj, "tv")
    return f"{{ x: {x} y: {y} z: {z} c: {color} u: {tu} v: {tv} }}"


def ref_summary(valobj, internal_dict):
    """Format o2::Ref<T>"""
    ptr = get_child_value(valobj, "mPtr")
    return f"{ptr}"


def weakref_summary(valobj, internal_dict):
    """Format o2::WeakRef<T>"""
    ptr_val = get_child_int(valobj, "mPtr")
    if ptr_val == 0:
        return "empty"
    else:
        ptr = get_child_value(valobj, "mPtr")
        return f"{ptr}"


def transform_summary(valobj, internal_dict):
    """Format o2::Transform"""
    pos = valobj.GetChildMemberWithName("mPosition")
    size = valobj.GetChildMemberWithName("mSize")
    scale = valobj.GetChildMemberWithName("mScale")
    pivot = valobj.GetChildMemberWithName("mPivot")
    angle = get_child_value(valobj, "mAngle")
    
    pos_str = vec2_summary(pos, {}) if pos.IsValid() else "?"
    size_str = vec2_summary(size, {}) if size.IsValid() else "?"
    scale_str = vec2_summary(scale, {}) if scale.IsValid() else "?"
    pivot_str = vec2_summary(pivot, {}) if pivot.IsValid() else "?"
    
    return f"(position={pos_str}, size={size_str}, scale={scale_str}, pivot={pivot_str}, angle={angle})"


def timestamp_summary(valobj, internal_dict):
    """Format o2::TimeStamp"""
    hour = get_child_value(valobj, "mHour")
    minute = get_child_value(valobj, "mMinute")
    second = get_child_value(valobj, "mSecond")
    day = get_child_value(valobj, "mDay")
    month = get_child_value(valobj, "mMonth")
    year = get_child_value(valobj, "mYear")
    return f"({hour}:{minute}:{second} {day}.{month}.{year})"


def widget_summary(valobj, internal_dict):
    """Format o2::Widget"""
    name = get_child_value(valobj, "mName")
    try:
        layout = valobj.GetChildMemberWithName("layout")
        if layout.IsValid():
            data = layout.GetChildMemberWithName("mData")
            if data.IsValid():
                rect = data.GetChildMemberWithName("worldRectangle")
                if rect.IsValid():
                    left = int(float(get_child_value(rect, "left")))
                    bottom = int(float(get_child_value(rect, "bottom")))
                    right = int(float(get_child_value(rect, "right")))
                    top = int(float(get_child_value(rect, "top")))
                    return f'Widget "{name}" ({left}, {bottom}, {right}, {top})'
    except:
        pass
    return f'Widget "{name}"'


def actor_summary(valobj, internal_dict):
    """Format o2::Actor"""
    name = get_child_value(valobj, "mName")
    id_val = get_child_value(valobj, "mId")
    return f'Actor "{name}":{id_val}'


def sprite_summary(valobj, internal_dict):
    """Format o2::Sprite"""
    pos = valobj.GetChildMemberWithName("mPosition")
    size = valobj.GetChildMemberWithName("mSize")
    scale = valobj.GetChildMemberWithName("mScale")
    pivot = valobj.GetChildMemberWithName("mPivot")
    angle = get_child_value(valobj, "mAngle")
    
    pos_str = vec2_summary(pos, {}) if pos.IsValid() else "?"
    size_str = vec2_summary(size, {}) if size.IsValid() else "?"
    scale_str = vec2_summary(scale, {}) if scale.IsValid() else "?"
    pivot_str = vec2_summary(pivot, {}) if pivot.IsValid() else "?"
    
    return f"(position={pos_str}, size={size_str}, scale={scale_str}, pivot={pivot_str}, angle={angle})"