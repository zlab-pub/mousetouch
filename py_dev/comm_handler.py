from struct import pack

class TypeID:
    POSITION = 0#'\x00'
    ANGLE = 1#'\x01'
    ANGLE_WHITE = 2
    ANGLE_COLOR2WHITE = 2#'\x02'
    ANGLE_COLOR2RED = 3#'\x03'


class TypeName:
    POSITION = 'POSITION'
    ANGLE = 'ANGLE'
    ANGLE_COLOR2WHITE = 'ANGLE_COLOR2WHITE'
    ANGLE_COLOR2RED = 'ANGLE_COLOR2RED'


def bytes_arr2_int_arr(bytes_arr):
    data = []
    for i in bytes_arr:
        data.append(int.from_bytes(i, 'big'))
    return data


def data_resolve(read_data):
    read_data = read_data.decode()
    type_id = read_data[0]
    data = []
    for i in read_data[1:].encode():
        data.append(int.from_bytes(i, 'big'))
    if type_id == TypeID.POSITION:
        return TypeName.POSITION, data
    elif type_id == TypeID.ANGLE:
        return TypeName.ANGLE, data
    return None, []


# return binary packet with 17 chars
# data_type: TypeName
# write_data: (loc1, loc2) or angle or dummy 0
def data_packing(data_type, write_data):
    packet = data_type.to_bytes(1,'big')
    if data_type == TypeID.POSITION:
        loc1 = write_data[2][0].to_bytes(2, 'little')
        loc2 = write_data[2][1].to_bytes(2, 'little')
        packet = packet + loc1 + loc2
    elif data_type == TypeID.ANGLE:
        angl = pack('f', write_data)
        packet = packet + angl
    elif data_type == TypeID.ANGLE_COLOR2RED: # or data_type == TypeID.ANGLE_COLOR2WHITE:
        packet = packet #+ (0).to_bytes(16, 'little')
    return packet


# return binary packet with 18 chars
def tagging_index(packet, index):
    return (chr(index).encode() + packet)
