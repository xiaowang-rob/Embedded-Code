/**
  ******************************************************************************
  * @file    usbd_audio.c
  * @author  MCD Application Team
  * @brief   This file provides the Audio core functions.
  *
  * @verbatim
  *
  *          ===================================================================
  *                                AUDIO Class  Description
  *          ===================================================================
  *           This driver manages the Audio Class 1.0 following the "USB Device Class Definition for
  *           Audio Devices V1.0 Mar 18, 98".
  *           This driver implements the following aspects of the specification:
  *             - Device descriptor management
  *             - Configuration descriptor management
  *             - Standard AC Interface Descriptor management
  *             - 1 Audio Streaming Interface (with single channel, PCM, Stereo mode)
  *             - 1 Audio Streaming Endpoint
  *             - 1 Audio Terminal Input (1 channel)
  *             - Audio Class-Specific AC Interfaces
  *             - Audio Class-Specific AS Interfaces
  *             - AudioControl Requests: only SET_CUR and GET_CUR requests are supported (for Mute)
  *             - Audio Feature Unit (limited to Mute control)
  *             - Audio Synchronization type: Asynchronous
  *             - Single fixed audio sampling rate (configurable in usbd_conf.h file)
  *          The current audio class version supports the following audio features:
  *             - Pulse Coded Modulation (PCM) format
  *             - sampling rate: 48KHz.
  *             - Bit resolution: 16
  *             - Number of channels: 2
  *             - No volume control
  *             - Mute/Unmute capability
  *             - Asynchronous Endpoints
  *
  * @note     In HS mode and when the DMA is used, all variables and data structures
  *           dealing with the DMA during the transaction process should be 32-bit aligned.
  *
  *
  *  @endverbatim
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                      www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* BSPDependencies
- "stm32xxxxx_{eval}{discovery}.c"
- "stm32xxxxx_{eval}{discovery}_io.c"
- "stm32xxxxx_{eval}{discovery}_audio.c"
EndBSPDependencies */

/* Includes ------------------------------------------------------------------*/
#include "usbd_audio.h"
#include "usbd_ctlreq.h"

/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_AUDIO
  * @brief usbd core module
  * @{
  */

/** @defgroup USBD_AUDIO_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_AUDIO_Private_Defines
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_AUDIO_Private_Macros
  * @{
  */
#define AUDIO_SAMPLE_FREQ(frq)         (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))

#define AUDIO_PACKET_SZE(frq)          (uint8_t)(((frq * 2U * 2U)/1000U) & 0xFFU), \
                                       (uint8_t)((((frq * 2U * 2U)/1000U) >> 8) & 0xFFU)

/**
  * @}
  */


/** @defgroup USBD_AUDIO_Private_FunctionPrototypes
  * @{
  */
static uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);

static uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req);

static uint8_t *USBD_AUDIO_GetCfgDesc(uint16_t *length);
static uint8_t *USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length);
static uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev);

static uint8_t USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);

/**
  * @}
  */

/** @defgroup USBD_AUDIO_Private_Variables
  * @{
  */

USBD_ClassTypeDef USBD_AUDIO =
{
  USBD_AUDIO_Init,
  USBD_AUDIO_DeInit,
  USBD_AUDIO_Setup,
  USBD_AUDIO_EP0_TxReady,
  USBD_AUDIO_EP0_RxReady,
  USBD_AUDIO_DataIn,
  USBD_AUDIO_DataOut,
  USBD_AUDIO_SOF,
  USBD_AUDIO_IsoINIncomplete,
  USBD_AUDIO_IsoOutIncomplete,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetDeviceQualifierDesc,
};

/* USB AUDIO device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_AUDIO_CfgDesc[USB_AUDIO_CONFIG_DESC_SIZ] __ALIGN_END =
    {
        /* Configuration 1 */
        0x09,                              /* bLength */
        USB_DESC_TYPE_CONFIGURATION,       /* bDescriptorType */
        LOBYTE(USB_AUDIO_CONFIG_DESC_SIZ), /* wTotalLength  109 bytes*/
        HIBYTE(USB_AUDIO_CONFIG_DESC_SIZ),
        0x02, /* bNumInterfaces */
        0x01, /* bConfigurationValue */
        0x00, /* iConfiguration */
#if (USBD_SELF_POWERED == 1U)
        0xC0, /* bmAttributes: Bus Powered according to user configuration */
#else
        0x80, /* bmAttributes: Bus Powered according to user configuration */
#endif
        USBD_MAX_POWER, /* bMaxPower = 100 mA */
        /* 09 byte*/

        /* USB Microphone Standard interface descriptor */
        AUDIO_INTERFACE_DESC_SIZE,   /* bLength */
        USB_DESC_TYPE_INTERFACE,     /* bDescriptorType */
        0x00,                        /* bInterfaceNumber */
        0x00,                        /* bAlternateSetting */
        0x00,                        /* bNumEndpoints */
        USB_DEVICE_CLASS_AUDIO,      /* bInterfaceClass */
        AUDIO_SUBCLASS_AUDIOCONTROL, /* bInterfaceSubClass */
        AUDIO_PROTOCOL_UNDEFINED,    /* bInterfaceProtocol */
        0x00,                        /* iInterface */
        /* 09 byte*/

        /* USB Microphone Class-specific AC Interface Descriptor */
        AUDIO_INTERFACE_DESC_SIZE,       /* bLength */
        AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
        AUDIO_CONTROL_HEADER,            /* bDescriptorSubtype */
        0x00, /* 1.00 */                 /* bcdADC */
        0x01,
        0x27, /* wTotalLength = 39*/
        0x00,
        0x01, /* bInCollection */
        0x01, /* baInterfaceNr */
        /* 09 byte*/

        /* USB Microphone Input Terminal Descriptor */
        AUDIO_INPUT_TERMINAL_DESC_SIZE,    /* bLength */
        AUDIO_INTERFACE_DESCRIPTOR_TYPE,   /* bDescriptorType */
        AUDIO_CONTROL_INPUT_TERMINAL,      /* bDescriptorSubtype */
        AUDIO_PORT_INPUT_TERMINAL_ID_1,    /* bTerminalID */
        AUDIO_PORT_MICRO_PHONE_TERMINAL_L, /* wTerminalType AUDIO_TERMINAL_USB_Microphone   0x0201 */
        AUDIO_PORT_MICRO_PHONE_TERMINAL_H,
        0x00,                        /* bAssocTerminal */
        AUDIO_PORT_CHANNEL_NUMS,     /* bNrChannels */
        AUDIO_PORT_CHANNEL_CONFIG_L, /* wChannelConfig 0x0000  Mono or L+R or L or R*/
        AUDIO_PORT_CHANNEL_CONFIG_H,
        0x00, /* iChannelNames */
        0x00, /* iTerminal */
        /* 12 byte*/

        /* USB Microphone Audio Feature Unit Descriptor */
        0x09,                            /* bLength */
        AUDIO_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType */
        AUDIO_CONTROL_FEATURE_UNIT,      /* bDescriptorSubtype */
        AUDIO_PORT_INPUT_CTL_ID_2,       /* bUnitID */
        AUDIO_PORT_INPUT_TERMINAL_ID_1,  /* bSourceID */
        0x01,                            /* bControlSize */
        AUDIO_CONTROL_MUTE,              /* bmaControls(0) */
        0,                               /* bmaControls(1) */
        0x00,                            /* iTerminal */
        /* 09 byte*/

        /*USB Microphone Output Terminal Descriptor */
        0x09,                              /* bLength */
        AUDIO_INTERFACE_DESCRIPTOR_TYPE,   /* bDescriptorType */
        AUDIO_CONTROL_OUTPUT_TERMINAL,     /* bDescriptorSubtype */
        AUDIO_PORT_OUTPUT_TERMINAL_ID_3,   /* bTerminalID */
        AUDIO_PORT_STREAM_TERMINAL_TYPE_L, /* wTerminalType  音频流0x0101*/
        AUDIO_PORT_STREAM_TERMINAL_TYPE_H,
        0x00,                      /* bAssocTerminal */
        AUDIO_PORT_INPUT_CTL_ID_2, /* bSourceID */
        0x00,                      /* iTerminal */
        /* 09 byte*/

        /* USB Microphone Standard AS Interface Descriptor - Audio Streaming Zero Bandwidth */
        /* Interface 1, Alternate Setting 0                                             */
        AUDIO_INTERFACE_DESC_SIZE,     /* bLength */
        USB_DESC_TYPE_INTERFACE,       /* bDescriptorType */
        0x01,                          /* bInterfaceNumber */
        0x00,                          /* bAlternateSetting */
        0x00,                          /* bNumEndpoints */
        USB_DEVICE_CLASS_AUDIO,        /* bInterfaceClass */
        AUDIO_SUBCLASS_AUDIOSTREAMING, /* bInterfaceSubClass */
        AUDIO_PROTOCOL_UNDEFINED,      /* bInterfaceProtocol */
        0x00,                          /* iInterface */
        /* 09 byte*/

        /* USB Microphone Standard AS Interface Descriptor - Audio Streaming Operational */
        /* Interface 1, Alternate Setting 1                                           */
        AUDIO_INTERFACE_DESC_SIZE,     /* bLength */
        USB_DESC_TYPE_INTERFACE,       /* bDescriptorType */
        0x01,                          /* bInterfaceNumber */
        0x01,                          /* bAlternateSetting */
        0x01,                          /* bNumEndpoints */
        USB_DEVICE_CLASS_AUDIO,        /* bInterfaceClass */
        AUDIO_SUBCLASS_AUDIOSTREAMING, /* bInterfaceSubClass */
        AUDIO_PROTOCOL_UNDEFINED,      /* bInterfaceProtocol */
        0x00,                          /* iInterface */
        /* 09 byte*/

        /* USB Microphone Audio Streaming Interface Descriptor */
        AUDIO_STREAMING_INTERFACE_DESC_SIZE, /* bLength */
        AUDIO_INTERFACE_DESCRIPTOR_TYPE,     /* bDescriptorType */
        AUDIO_STREAMING_GENERAL,             /* bDescriptorSubtype */
        AUDIO_PORT_OUTPUT_TERMINAL_ID_3,     /* bTerminalLink */
        0x01,                                /* bDelay */
        0x01,                                /* wFormatTag AUDIO_FORMAT_PCM  0x0001 */
        0x00,
        /* 07 byte*/

        /* USB Microphone Audio Type III Format Interface Descriptor */
        0x0B,                               /* bLength */
        AUDIO_INTERFACE_DESCRIPTOR_TYPE,    /* bDescriptorType */
        AUDIO_STREAMING_FORMAT_TYPE,        /* bDescriptorSubtype */
        AUDIO_FORMAT_TYPE_I,                /* bFormatType */
        AUDIO_PORT_CHANNEL_NUMS,            /* bNrChannels */
        0x02,                               /* bSubFrameSize :  2 Bytes per frame (16bits) */
        16,                                 /* bBitResolution (16-bits per sample) */
        0x01,                               /* bSamFreqType only one frequency supported */
        AUDIO_SAMPLE_FREQ(USBD_AUDIO_FREQ), /* Audio sampling frequency coded on 3 bytes */
        /* 11 byte*/

        /* Endpoint 1 - Standard Descriptor */
        AUDIO_STANDARD_ENDPOINT_DESC_SIZE,      /* bLength */
        USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType */
        AUDIO_PORT_IN_EP_DIR_ID,                /* bEndpointAddress 1 in endpoint */
        USBD_EP_TYPE_ISOC,                      /* bmAttributes */
        AUDIO_PORT_PACKET_SZE(USBD_AUDIO_FREQ), /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*2(HalfWord)) */
        AUDIO_PORT_FS_BINTERVAL,                /* bInterval */
        0x00,                                   /* bRefresh */
        0x00,                                   /* bSynchAddress */
        /* 09 byte*/

        /* Endpoint - Audio Streaming Descriptor*/
        AUDIO_STREAMING_ENDPOINT_DESC_SIZE, /* bLength */
        AUDIO_ENDPOINT_DESCRIPTOR_TYPE,     /* bDescriptorType */
        AUDIO_ENDPOINT_GENERAL,             /* bDescriptor */
        0x00,                               /* bmAttributes */
        0x00,                               /* bLockDelayUnits */
        0x00,                               /* wLockDelay */
        0x00,
        /* 07 byte*/
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_AUDIO_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

/**
  * @}
  */

/** @defgroup USBD_AUDIO_Private_Functions
  * @{
  */

/**
  * @brief  USBD_AUDIO_Init
  *         Initialize the AUDIO interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  return USB_Audio_Port_EP_IN_Init(pdev, cfgidx);
}

/**
  * @brief  USBD_AUDIO_Init
  *         DeInitialize the AUDIO layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  UNUSED(cfgidx);

  /* Open EP OUT */
  (void)USBD_LL_CloseEP(pdev, AUDIO_OUT_EP);
  pdev->ep_out[AUDIO_OUT_EP & 0xFU].is_used = 0U;
  pdev->ep_out[AUDIO_OUT_EP & 0xFU].bInterval = 0U;

  /* DeInit  physical Interface components */
  if (pdev->pClassData != NULL)
  {
    ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->DeInit(0U);
    (void)USBD_free(pdev->pClassData);
    pdev->pClassData = NULL;
  }

  return (uint8_t)USBD_OK;
}

/**
  * @brief  USBD_AUDIO_Setup
  *         Handle the AUDIO specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  uint16_t len;
  uint8_t *pbuf;
  uint16_t status_info = 0U;
  USBD_StatusTypeDef ret = USBD_OK;

  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassData;

  if (haudio == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
    case USB_REQ_TYPE_CLASS:
      switch (req->bRequest)
      {
        case AUDIO_REQ_GET_CUR:
          AUDIO_REQ_GetCurrent(pdev, req);
          break;

        case AUDIO_REQ_SET_CUR:
          AUDIO_REQ_SetCurrent(pdev, req);
          break;

        default:
          USBD_CtlError(pdev, req);
          ret = USBD_FAIL;
          break;
      }
      break;

    case USB_REQ_TYPE_STANDARD:
      switch (req->bRequest)
      {
        case USB_REQ_GET_STATUS:
          if (pdev->dev_state == USBD_STATE_CONFIGURED)
          {
            (void)USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
          }
          else
          {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_GET_DESCRIPTOR:
          if ((req->wValue >> 8) == AUDIO_DESCRIPTOR_TYPE)
          {
            pbuf = USBD_AUDIO_CfgDesc + 18;
            len = MIN(USB_AUDIO_DESC_SIZ, req->wLength);

            (void)USBD_CtlSendData(pdev, pbuf, len);
          }
          break;

        case USB_REQ_GET_INTERFACE:
          if (pdev->dev_state == USBD_STATE_CONFIGURED)
          {
            (void)USBD_CtlSendData(pdev, (uint8_t *)&haudio->alt_setting, 1U);
          }
          else
          {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_SET_INTERFACE:
          if (pdev->dev_state == USBD_STATE_CONFIGURED)
          {
            if ((uint8_t)(req->wValue) <= USBD_MAX_NUM_INTERFACES)
            {
              haudio->alt_setting = (uint8_t)(req->wValue);
            }
            else
            {
              /* Call the error management function (command will be NAKed */
              USBD_CtlError(pdev, req);
              ret = USBD_FAIL;
            }
          }
          else
          {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_CLEAR_FEATURE:
          break;

        default:
          USBD_CtlError(pdev, req);
          ret = USBD_FAIL;
          break;
      }
      break;
    default:
      USBD_CtlError(pdev, req);
      ret = USBD_FAIL;
      break;
  }

  return (uint8_t)ret;
}


/**
  * @brief  USBD_AUDIO_GetCfgDesc
  *         return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t *USBD_AUDIO_GetCfgDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_AUDIO_CfgDesc);

  return USBD_AUDIO_CfgDesc;
}

/**
  * @brief  USBD_AUDIO_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  return USB_Audio_Port_DataIn(pdev, epnum);
}

/**
  * @brief  USBD_AUDIO_EP0_RxReady
  *         handle EP0 Rx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassData;

  if (haudio == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  if (haudio->control.cmd == AUDIO_REQ_SET_CUR)
  {
    /* In this driver, to simplify code, only SET_CUR request is managed */

    if (haudio->control.unit == AUDIO_OUT_STREAMING_CTRL)
    {
      ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->MuteCtl(haudio->control.data[0]);
      haudio->control.cmd = 0U;
      haudio->control.len = 0U;
    }
  }

  return (uint8_t)USBD_OK;
}
/**
  * @brief  USBD_AUDIO_EP0_TxReady
  *         handle EP0 TRx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev)
{
  UNUSED(pdev);

  /* Only OUT control data are processed */
  return (uint8_t)USBD_OK;
}
/**
  * @brief  USBD_AUDIO_SOF
  *         handle SOF event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev)
{
  UNUSED(pdev);

  return (uint8_t)USBD_OK;
}

/**
  * @brief  USBD_AUDIO_SOF
  *         handle SOF event
  * @param  pdev: device instance
  * @retval status
  */
void USBD_AUDIO_Sync(USBD_HandleTypeDef *pdev, AUDIO_OffsetTypeDef offset)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  uint32_t BufferSize = AUDIO_TOTAL_BUF_SIZE / 2U;

  if (pdev->pClassData == NULL)
  {
    return;
  }

  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassData;

  haudio->offset = offset;

  if (haudio->rd_enable == 1U)
  {
    haudio->rd_ptr += (uint16_t)BufferSize;

    if (haudio->rd_ptr == AUDIO_TOTAL_BUF_SIZE)
    {
      /* roll back */
      haudio->rd_ptr = 0U;
    }
  }

  if (haudio->rd_ptr > haudio->wr_ptr)
  {
    if ((haudio->rd_ptr - haudio->wr_ptr) < AUDIO_OUT_PACKET)
    {
      BufferSize += 4U;
    }
    else
    {
      if ((haudio->rd_ptr - haudio->wr_ptr) > (AUDIO_TOTAL_BUF_SIZE - AUDIO_OUT_PACKET))
      {
        BufferSize -= 4U;
      }
    }
  }
  else
  {
    if ((haudio->wr_ptr - haudio->rd_ptr) < AUDIO_OUT_PACKET)
    {
      BufferSize -= 4U;
    }
    else
    {
      if ((haudio->wr_ptr - haudio->rd_ptr) > (AUDIO_TOTAL_BUF_SIZE - AUDIO_OUT_PACKET))
      {
        BufferSize += 4U;
      }
    }
  }

  if (haudio->offset == AUDIO_OFFSET_FULL)
  {
    ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->AudioCmd(&haudio->buffer[0],
                                                         BufferSize, AUDIO_CMD_PLAY);
    haudio->offset = AUDIO_OFFSET_NONE;
  }
}

/**
  * @brief  USBD_AUDIO_IsoINIncomplete
  *         handle data ISO IN Incomplete event
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t USBD_AUDIO_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  UNUSED(pdev);
  UNUSED(epnum);

  return (uint8_t)USBD_OK;
}
/**
  * @brief  USBD_AUDIO_IsoOutIncomplete
  *         handle data ISO OUT Incomplete event
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  UNUSED(pdev);
  UNUSED(epnum);

  return (uint8_t)USBD_OK;
}
/**
  * @brief  USBD_AUDIO_DataOut
  *         handle data OUT Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  uint16_t PacketSize;
  USBD_AUDIO_HandleTypeDef *haudio;

  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassData;

  if (haudio == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  if (epnum == AUDIO_OUT_EP)
  {
    /* Get received data packet length */
    PacketSize = (uint16_t)USBD_LL_GetRxDataSize(pdev, epnum);

    /* Packet received Callback */
    ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->PeriodicTC(&haudio->buffer[haudio->wr_ptr],
                                                           PacketSize, AUDIO_OUT_TC);

    /* Increment the Buffer pointer or roll it back when all buffers are full */
    haudio->wr_ptr += PacketSize;

    if (haudio->wr_ptr == AUDIO_TOTAL_BUF_SIZE)
    {
      /* All buffers are full: roll back */
      haudio->wr_ptr = 0U;

      if (haudio->offset == AUDIO_OFFSET_UNKNOWN)
      {
        ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->AudioCmd(&haudio->buffer[0],
                                                             AUDIO_TOTAL_BUF_SIZE / 2U,
                                                             AUDIO_CMD_START);
        haudio->offset = AUDIO_OFFSET_NONE;
      }
    }

    if (haudio->rd_enable == 0U)
    {
      if (haudio->wr_ptr == (AUDIO_TOTAL_BUF_SIZE / 2U))
      {
        haudio->rd_enable = 1U;
      }
    }

    /* Prepare Out endpoint to receive next audio packet */
    (void)USBD_LL_PrepareReceive(pdev, AUDIO_OUT_EP,
                                 &haudio->buffer[haudio->wr_ptr],
                                 AUDIO_OUT_PACKET);
  }

  return (uint8_t)USBD_OK;
}

/**
  * @brief  AUDIO_Req_GetCurrent
  *         Handles the GET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassData;

  if (haudio == NULL)
  {
    return;
  }

  (void)USBD_memset(haudio->control.data, 0, 64U);

  /* Send the current mute state */
  (void)USBD_CtlSendData(pdev, haudio->control.data, req->wLength);
}

/**
  * @brief  AUDIO_Req_SetCurrent
  *         Handles the SET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef *)pdev->pClassData;

  if (haudio == NULL)
  {
    return;
  }

  if (req->wLength != 0U)
  {
    /* Prepare the reception of the buffer over EP0 */
    (void)USBD_CtlPrepareRx(pdev, haudio->control.data, req->wLength);

    haudio->control.cmd = AUDIO_REQ_SET_CUR;     /* Set the request value */
    haudio->control.len = (uint8_t)req->wLength; /* Set the request data length */
    haudio->control.unit = HIBYTE(req->wIndex);  /* Set the request target unit */
  }
}


/**
  * @brief  DeviceQualifierDescriptor
  *         return Device Qualifier descriptor
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t *USBD_AUDIO_GetDeviceQualifierDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_AUDIO_DeviceQualifierDesc);

  return USBD_AUDIO_DeviceQualifierDesc;
}

/**
  * @brief  USBD_AUDIO_RegisterInterface
  * @param  fops: Audio interface callback
  * @retval status
  */
uint8_t USBD_AUDIO_RegisterInterface(USBD_HandleTypeDef *pdev,
                                     USBD_AUDIO_ItfTypeDef *fops)
{
  if (fops == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  pdev->pUserData = fops;

  return (uint8_t)USBD_OK;
}
/**
  * @}
  */


/**
  * @}
  */


/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
