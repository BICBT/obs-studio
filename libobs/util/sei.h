//
// Created by lingpan on 2021/1/11.
//

#ifndef OBS_STUDIO_SEI_H
#define OBS_STUDIO_SEI_H

#endif //OBS_STUDIO_SEI_H

#pragma once

#define UUID_SIZE 16
static unsigned char uuid[] = { 0x54, 0x80, 0x83, 0x97, 0xf0, 0x23, 0x47, 0x4b, 0xb7, 0xf7, 0x4f, 0x32, 0xb5, 0x4e, 0x06, 0xac };

typedef struct sei_content {
        uint8_t * uuid;
        uint8_t * data;
        uint32_t size;
        uint32_t payload_size;
}sei_content;

uint32_t reversebytes(uint32_t value) {
        return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 |
               (value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24;
}

//获取SEI内容
static int32_t get_sei_buffer(uint8_t * packet, uint32_t size, sei_content *content)
{
        if (size <= 0) return -1;
        if (packet == NULL) return -1;

        uint8_t * sei = packet;
        uint32_t sei_type = 0;
        uint32_t sei_size = 0;
        //payload type
        do {
                sei_type += *sei;
        } while (*sei++ == 255);
        //数据长度
        do {
                sei_size += *sei;
        } while (*sei++ == 255);

        //检查UUID
        if (sei_size >= UUID_SIZE && sei_size <= (packet + size - sei) &&
            sei_type == 5)
        {
                content->uuid = sei;

                sei += UUID_SIZE;
                sei_size -= UUID_SIZE;

                content->data = sei;
                content->size = (uint32_t)(packet + size - sei);
                content->payload_size = sei_size;

                return sei_size;
        }
        return -1;
}

//检测非竞争长度
static uint32_t get_content_uncompete_size(const uint8_t *data, uint32_t size)
{
        if (data == NULL) return 0;

        uint32_t zero_count = 0;
        uint32_t uncompete_size = size;
        for (uint32_t i = 0; i < size; i++)
        {
                if (zero_count >= 2)
                {
                        if (data[i] == 0x03)
                        {
                                uncompete_size -= 1;
                        }
                        zero_count = 0;
                }
                else if (data[i] == 0x00)
                {
                        zero_count++;
                }
                else
                {
                        zero_count = 0;
                }
        }
        return uncompete_size;
}

//反解数据
static void uncompete_content(const sei_content * content, uint8_t * data, uint32_t size)
{
        if (size == content->size)
        {
                memcpy(data, content->data, size);
        }
        else
        {
                int zero_count = 0;
                for (uint32_t i = 0; i < content->size; i++)
                {
                        if (zero_count >= 2)
                        {
                                if (content->data[i] == 0x03)
                                {
                                        zero_count = 0;
                                }
                                else
                                {
                                        *data++ = content->data[i];
                                        size--;
                                }
                        }
                        else if (content->data[i] == 0x00)
                        {
                                zero_count++;
                                *data++ = content->data[i];
                                size--;
                        }
                        else
                        {
                                zero_count = 0;
                                *data++ = content->data[i];
                                size--;
                        }
                        if (size <= 0) break;
                }
        }
}

//获取标准H264头类型
uint32_t get_annexb_size(uint8_t * packet, uint32_t size)
{
        unsigned char ANNEXB_CODE_LOW[] = { 0x00,0x00,0x01 };
        unsigned char ANNEXB_CODE[] = { 0x00,0x00,0x00,0x01 };

        unsigned char *data = packet;
        if (data == NULL) return 0;
        if (size > 3 && memcmp(data, ANNEXB_CODE_LOW, 3) == 0)
        {
                return 3;
        }
        else if (size > 4 && memcmp(data, ANNEXB_CODE, 4) == 0)
        {
                return 4;
        }
        return 0;
}

//搜索标准头
int32_t find_annexb(uint8_t * packet, uint32_t size)
{
        uint8_t * data = packet;
        if (size <= 0) return -1;
        int32_t index = 0;
        while (size - index > 0)
        {
                if ((size - index > 3) && data[index] == 0x00 && data[index + 1] == 0x00)
                {
                        if (data[index + 2] == 0x01)
                        {
                                return index;
                        }
                        else if ((size - index > 4) && data[index + 2] == 0x00 && data[index + 3] == 0x01)
                        {
                                return index;
                        }
                }
                index += 1;
        }

        return -1;
}

//检查是否为标准H264
uint32_t check_is_annexb(uint8_t * packet, uint32_t size)
{
        unsigned char ANNEXB_CODE_LOW[] = { 0x00,0x00,0x01 };
        unsigned char ANNEXB_CODE[] = { 0x00,0x00,0x00,0x01 };

        unsigned char *data = packet;
        if (data == NULL) return 0;
        uint32_t isAnnexb = 0;
        if ((size > 3 && memcmp(data, ANNEXB_CODE_LOW, 3) == 0) ||
            (size > 4 && memcmp(data, ANNEXB_CODE, 4) == 0)
                )
        {
                isAnnexb = 1;
        }
        return isAnnexb;
}

int32_t get_annexb_sei_content(uint8_t * packet, uint32_t size, const uint8_t *uuid, uint8_t ** pdata, uint32_t *psize)
{
        uint8_t * data = packet;
        uint32_t data_size = size;

        uint8_t * nalu_element = NULL;
        int32_t nalu_element_size = 0;
        while (data < packet + size) {
                int32_t index = find_annexb(data, data_size);
                int32_t second_index = 0;
                if (index != -1)
                {
                        uint32_t startCodeSize = get_annexb_size(data + index, data_size - index);
                        second_index = find_annexb(data + index + startCodeSize, data_size - index - startCodeSize);
                        if (second_index >= 0)
                        {
                                second_index += startCodeSize;
                        }
                }

                if (index != -1)
                {
                        if (second_index == -1)
                        {
                                second_index = data_size;
                        }
                        nalu_element = data + index;
                        nalu_element_size = second_index;
                        data += second_index;
                        data_size -= second_index;
                }
                else
                {
                        return -1;
                }
                if (nalu_element != NULL && nalu_element_size != 0)
                {
                        if ((int32_t)(packet + size - nalu_element) < nalu_element_size)
                        {
                                nalu_element_size = (int32_t)(packet + size - nalu_element);
                        }

                        uint32_t startCodeSize = get_annexb_size(nalu_element, nalu_element_size);
                        if (startCodeSize == 0) continue;
                        //SEI
                        if ((nalu_element[startCodeSize] & 0x1F) == 6)
                        {
                                uint8_t * sei_data = nalu_element + startCodeSize + 1;
                                int32_t sei_data_length = nalu_element_size - startCodeSize - 1;
                                sei_content content = { 0 };
                                int32_t ret = get_sei_buffer(sei_data, sei_data_length, &content);
                                if (ret != -1)
                                {
                                        if (memcmp(uuid, content.uuid, UUID_SIZE) == 0)
                                        {
                                                int32_t uncompete_size = get_content_uncompete_size(content.data, content.size);
                                                if (uncompete_size > 0 && pdata != NULL)
                                                {
                                                        uncompete_size = min((int32_t)content.payload_size, uncompete_size);
                                                        uint8_t * outData = (uint8_t*)malloc(uncompete_size + 1);
                                                        memset(outData, 0, uncompete_size + 1);
                                                        uncompete_content(&content, outData, uncompete_size);
                                                        if (pdata != NULL)
                                                        {
                                                                *pdata = outData;
                                                        }
                                                        else
                                                        {
                                                                free(outData);
                                                        }
                                                }
                                                if (psize != NULL) *psize = uncompete_size;
                                                return uncompete_size;
                                        }
                                }
                        }
                }
        }
        return -1;
}


int32_t get_mp4_sei_content(uint8_t * packet, uint32_t size, const uint8_t *uuid, uint8_t ** pdata, uint32_t *psize)
{
        //return -1;
        uint8_t * data = packet;
        //当前NALU
        while (data < packet + size) {
                //MP4格式起始码/长度
                uint32_t *nalu_length = (uint32_t *)data;
                uint32_t nalu_size = reversebytes(*nalu_length);
                //NALU header
                if ((*(data + 4) & 0x1F) == 6)
                {
                        //SEI
                        uint8_t * sei_data = data + 4 + 1;
                        uint32_t sei_data_length = min(nalu_size, (uint32_t)(packet + size - sei_data));
                        sei_content content = { 0 };
                        int ret = get_sei_buffer(sei_data, sei_data_length, &content);
                        if (ret != -1)
                        {
                                if (memcmp(uuid, content.uuid, UUID_SIZE) == 0)
                                {
                                        uint32_t uncompete_size = get_content_uncompete_size(content.data, content.size);
                                        if (uncompete_size > 0 && pdata != NULL)
                                        {
                                                uncompete_size = min(content.payload_size, uncompete_size);
                                                uint8_t * outData = (uint8_t*)malloc(uncompete_size + 1);
                                                memset(outData, 0, uncompete_size + 1);
                                                uncompete_content(&content, outData, uncompete_size);
                                                if (pdata != NULL)
                                                {
                                                        *pdata = outData;
                                                }
                                                else
                                                {
                                                        free(outData);
                                                }
                                        }
                                        if (psize != NULL) *psize = uncompete_size;
                                        return uncompete_size;
                                }
                        }
                }
                data += 4 + nalu_size;
        }
        return -1;
}

int get_sei_content(uint8_t * packet, uint32_t size, const uint8_t *uuid, uint8_t ** pdata, uint32_t *psize)
{
        if (uuid == NULL) return -1;
        uint32_t isAnnexb = check_is_annexb(packet, size);
        //暂时只处理MP4封装,annexb暂为处理
        if (isAnnexb)
        {
                return get_annexb_sei_content(packet, size, uuid, pdata, psize);
        }
        else
        {
                return get_mp4_sei_content(packet, size, uuid, pdata, psize);
        }
        return -1;
}

