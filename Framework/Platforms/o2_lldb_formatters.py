#!/usr/bin/env python3
"""
LLDB Formatters for o2 Framework Types  
Based on Framework.natvis
"""

import lldb
import struct
import traceback

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
    
    # DataValue formatter
    debugger.HandleCommand('type summary add -F o2_lldb_formatters.datavalue_summary "o2::DataValue"')
    debugger.HandleCommand('type synthetic add -l o2_lldb_formatters.DataValueSyntheticProvider "o2::DataValue"')
    
    # DataMember formatter
    debugger.HandleCommand('type summary add -F o2_lldb_formatters.datamember_summary "o2::DataMember"')
    
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


def datamember_summary(valobj, internal_dict):
    """Format o2::DataMember"""
    try:
        target = valobj.GetTarget()
        datavalue_type = target.FindFirstType("o2::DataValue")
        if not datavalue_type.IsValid():
            return "DataMember(? no type)"
        
        datavalue_size = datavalue_type.GetByteSize()
        
        data_addr = valobj.GetLoadAddress()
        if data_addr == lldb.LLDB_INVALID_ADDRESS:
            addr = valobj.GetAddress()
            if addr.IsValid():
                data_addr = addr.GetLoadAddress(target)
        
        if data_addr == lldb.LLDB_INVALID_ADDRESS:
            return "DataMember(? no addr)"
        
        name = valobj.CreateValueFromAddress("name", data_addr, datavalue_type)
        value = valobj.CreateValueFromAddress("value", data_addr + datavalue_size, datavalue_type)
        
        if name.IsValid() and value.IsValid():
            name_str = datavalue_summary(name, {})
            value_str = datavalue_summary(value, {})
            return f"{{ {name_str} : {value_str} }}"
        
        return "DataMember(? invalid)"
    except Exception as e:
        return f"DataMember(? {str(e)[:30]})"


def datavalue_summary(valobj, internal_dict):
    """Format o2::DataValue"""
    debug = False
    log = []
    
    try:
        error = lldb.SBError()
        process = valobj.GetProcess()
        
        data_addr = valobj.GetLoadAddress()
        if data_addr == lldb.LLDB_INVALID_ADDRESS:
            addr = valobj.GetAddress()
            if addr.IsValid():
                data_addr = addr.GetLoadAddress(valobj.GetTarget())
        
        if data_addr == lldb.LLDB_INVALID_ADDRESS:
            return "? (no addr)"
        
        if debug:
            log.append(f"data_addr={hex(data_addr)}")
        
        flags_offset = 16
        flags_addr = data_addr + flags_offset
        flags_data = process.ReadUnsignedFromMemory(flags_addr, 4, error)
        
        if not error.Success():
            return f"? (flags read error: {error.GetCString()})"
        
        flags = flags_data
        
        if debug:
            log.append(f"flags={hex(flags)}")
        
        if flags == 0:
            return "? (flags=0)"
        
        Null = 1 << 10
        BoolTrue = 1 << 11
        BoolFalse = 1 << 12
        ShortString = 1 << 13
        StringRef = 1 << 14
        StringCopy = 1 << 15
        Int = 1 << 1
        UInt = 1 << 2
        Int64 = 1 << 3
        UInt64 = 1 << 4
        Double = 1 << 5
        Object = 1 << 8
        Array = 1 << 9
        
        if flags & Null:
            return "null"
        
        if flags & BoolTrue:
            return "true"
        
        if flags & BoolFalse:
            return "false"
        
        if flags & ShortString:
            result = process.ReadCStringFromMemory(data_addr, 15, error)
            if error.Success() and result:
                return f'"{result}"'
            return f'? (ShortString error: {error.GetCString() if not error.Success() else "empty"})'
        
        if flags & (StringRef | StringCopy):
            ptr_val = process.ReadPointerFromMemory(data_addr, error)
            if not error.Success():
                return f'? (StringPtr read error: {error.GetCString()})'
            if ptr_val == 0:
                return '? (StringPtr null)'
            result = process.ReadCStringFromMemory(ptr_val, 256, error)
            if error.Success() and result:
                return f'"{result}"'
            return f'? (String read error: {error.GetCString() if not error.Success() else "empty"})'
        
        if flags & Int:
            int_val = process.ReadSignedFromMemory(data_addr, 4, error)
            if error.Success():
                return str(int_val)
            return f"? (Int error: {error.GetCString()})"
        
        if flags & UInt:
            uint_val = process.ReadUnsignedFromMemory(data_addr, 4, error)
            if error.Success():
                return str(uint_val)
            return f"? (UInt error: {error.GetCString()})"
        
        if flags & Int64:
            int64_val = process.ReadSignedFromMemory(data_addr, 8, error)
            if error.Success():
                return str(int64_val)
            return f"? (Int64 error: {error.GetCString()})"
        
        if flags & UInt64:
            uint64_val = process.ReadUnsignedFromMemory(data_addr, 8, error)
            if error.Success():
                return str(uint64_val)
            return f"? (UInt64 error: {error.GetCString()})"
        
        if flags & Double:
            data = process.ReadMemory(data_addr, 8, error)
            if error.Success():
                double_val = struct.unpack('d', data)[0]
                return str(double_val)
            return f"? (Double error: {error.GetCString()})"
        
        if flags & Object:
            count = process.ReadUnsignedFromMemory(data_addr + 8, 4, error)
            if error.Success():
                return f"Object members={count}"
            return f"Object members=? (error: {error.GetCString()})"
        
        if flags & Array:
            count = process.ReadUnsignedFromMemory(data_addr + 8, 4, error)
            if error.Success():
                return f"Array elements={count}"
            return f"Array elements=? (error: {error.GetCString()})"
        
        return f"? (no matching flags: {hex(flags)})"
    except Exception as e:
        tb = traceback.format_exc()
        return f"? (exception: {str(e)[:50]})"


class DataValueSyntheticProvider:
    """Synthetic children provider for o2::DataValue"""
    
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        self.update()
    
    def update(self):
        """Called when the variable changes"""
        try:
            error = lldb.SBError()
            process = self.valobj.GetProcess()
            
            data_addr = self.valobj.GetLoadAddress()
            if data_addr == lldb.LLDB_INVALID_ADDRESS:
                addr = self.valobj.GetAddress()
                if addr.IsValid():
                    data_addr = addr.GetLoadAddress(self.valobj.GetTarget())
            
            if data_addr == lldb.LLDB_INVALID_ADDRESS:
                self.count = 0
                self.is_object = False
                self.is_array = False
                return
            
            flags_offset = 16
            flags_addr = data_addr + flags_offset
            flags = process.ReadUnsignedFromMemory(flags_addr, 4, error)
            
            if not error.Success():
                self.count = 0
                self.is_object = False
                self.is_array = False
                return
            
            Object = 1 << 8
            Array = 1 << 9
            
            self.is_object = bool(flags & Object)
            self.is_array = bool(flags & Array)
            
            if self.is_object:
                ptr_addr = process.ReadPointerFromMemory(data_addr, error)
                if error.Success() and ptr_addr != 0:
                    self.count = process.ReadUnsignedFromMemory(data_addr + 8, 4, error)
                    if error.Success():
                        self.members_addr = ptr_addr
                    else:
                        self.count = 0
                else:
                    self.count = 0
            elif self.is_array:
                ptr_addr = process.ReadPointerFromMemory(data_addr, error)
                if error.Success() and ptr_addr != 0:
                    self.count = process.ReadUnsignedFromMemory(data_addr + 8, 4, error)
                    if error.Success():
                        self.elements_addr = ptr_addr
                    else:
                        self.count = 0
                else:
                    self.count = 0
            else:
                self.count = 0
        except:
            self.count = 0
            self.is_object = False
            self.is_array = False
    
    def num_children(self):
        """Return number of children"""
        return self.count
    
    def has_children(self):
        """Return whether this value has children"""
        return self.count > 0
    
    def get_child_index(self, name):
        """Return index of child with given name"""
        try:
            if name.startswith('[') and name.endswith(']'):
                return int(name[1:-1])
        except:
            pass
        return -1
    
    def get_child_at_index(self, index):
        """Return child at given index"""
        if index < 0 or index >= self.count:
            return None
        
        try:
            target = self.valobj.GetTarget()
            
            if self.is_object:
                if not hasattr(self, 'members_addr') or self.members_addr == 0:
                    return None
                
                member_type = target.FindFirstType("o2::DataMember")
                if not member_type.IsValid():
                    return None
                
                member_size = member_type.GetByteSize()
                offset = index * member_size
                member_addr = self.members_addr + offset
                
                return self.valobj.CreateValueFromAddress(f'[{index}]', member_addr, member_type)
            
            elif self.is_array:
                if not hasattr(self, 'elements_addr') or self.elements_addr == 0:
                    return None
                
                element_type = target.FindFirstType("o2::DataValue")
                if not element_type.IsValid():
                    return None
                
                element_size = element_type.GetByteSize()
                offset = index * element_size
                element_addr = self.elements_addr + offset
                
                return self.valobj.CreateValueFromAddress(f'[{index}]', element_addr, element_type)
        except:
            pass
        
        return None