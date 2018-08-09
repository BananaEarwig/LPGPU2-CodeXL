//==============================================================================
// Copyright (c) 2013-2015 Advanced Micro Devices, Inc. All rights reserved.
/// \author AMD Developer Tools Team
/// \file
/// \brief  Class to implement handling of HTTP requests and communication
///         between the client and server
//==============================================================================

#ifndef GPS_HTTPREQUEST_H
#define GPS_HTTPREQUEST_H

#include "NetSocket.h"
#include <string>
#include "ICommunication.h"
#include "defines.h"
#include "malloc.h"
#include "Logger.h"
#include "stdio.h"
#include <AMDTBaseTools/Include/gtASCIIString.h>
#include "SharedMemoryManager.h"
#include "misc.h"
#include <AMDTOSWrappers/Include/osProcess.h>

#define COMM_BUFFER_SIZE 8192 ///< Max comms buffer size
#define SMALL_BUFFER_SIZE 10 ///< Samll buffer size
#define COMM_MAX_URL_SIZE 8192 ///< Max comms URL size
#define COMM_MAX_STREAM_RATE UINT_MAX ///< Max comms stream rate

/// Error return codes for the ReadWebRequest method.
enum HTTP_REQUEST_RESULT
{
    /// Web request was processed successfully.
    HTTP_NO_ERROR = 0,
    /// Error reading from the socket.
    HTTP_SOCKET_ERROR,
    /// Error parsing the web request data read from the socket.
    HTTP_PARSE_ERROR,
    /// Error reading the POST data.
    HTTP_POST_DATA_ERROR
};

#if defined (_WIN32)
    typedef  IN_ADDR     SockAddrIn;    ///< Socket address
#else
    typedef  in_addr     SockAddrIn;    ///< Socket address
#endif // _WIN32

//////////////////////////////////////////////////////////////////////////////////////////////////////
/// Responsible for storing the data in a HTTP request header.
/// The HTTPHeaderData is the only data we send across the shared memory system.
//////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    /// used to identify which message/command this data is associated with
    CommunicationID handle;

    /// Either GET or PUT; currently only GET is supported by PS2
    char method[ SMALL_BUFFER_SIZE ];

    /// The requested URL
    char url[ COMM_MAX_URL_SIZE ];

    /// The http version that is being used
    char httpversion[ SMALL_BUFFER_SIZE ];

    /// Indicates that this request was received over a socket. Due to the communication
    /// channels in PerfStudio, these Request headers may also be duplicated across processes.
    bool bReceivedOverSocket;

    /// the IP address of the client which is sending the request
    SockAddrIn client_ip;

    /// information about the protocol used.
    NetSocket::DuplicationInfo ProtoInfo;

    /// The port to use for this request
    DWORD dwPort;

    /// The size of the data passed in POST
    unsigned int nPostDataSize;

} HTTPHeaderData;

//////////////////////////////////////////////////////////////////////////////////////////////////////
/// Wrapper object for the HTTPHeaderData
/// This provides C++ wrapper for the HTTPHeaderData payload struct.
//////////////////////////////////////////////////////////////////////////////////////////////////////
class HTTPRequestHeader
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Default constructor.
    /// Clears the data members
    ////////////////////////////////////////////////////////////////////////////////////////////
    HTTPRequestHeader()
        :  m_pPostData(NULL)
    {
        InitializeHeaderData();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructor that uses an already existing HTTPHeaderData
    /// We only pass HTTPHeaderData structs across the shared memory,
    /// so we use this to re-construct a fully working object.
    /// \param httpHeaderData The input data to initialize with.
    ////////////////////////////////////////////////////////////////////////////////////////////
    HTTPRequestHeader(HTTPHeaderData& httpHeaderData)
        :  m_pPostData(NULL)
    {
        m_httpHeaderData = httpHeaderData;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Destructor
    ////////////////////////////////////////////////////////////////////////////////////////////
    ~HTTPRequestHeader()
    {
        // Clear the POST data
        if (m_pPostData != NULL && m_httpHeaderData.nPostDataSize > 0)
        {
            free(m_pPostData);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Set bReceivedOverSocket member.
    /// \param bFlag The current state.
    ////////////////////////////////////////////////////////////////////////////////////////////
    void SetReceivedOverSocket(bool bFlag);

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Get the bReceivedOverSocket member.
    /// \return true or false.
    ////////////////////////////////////////////////////////////////////////////////////////////
    bool GetReceivedOverSocket();

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Get the client socket handle.
    /// \param handle the handle used to identify this data.
    ////////////////////////////////////////////////////////////////////////////////////////////
    void SetClientHandle(CommunicationID handle);

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Set client_ip member.
    /// \param clientIP The client IP
    ////////////////////////////////////////////////////////////////////////////////////////////
    void SetClientIP(SockAddrIn clientIP);

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Get the client IP address.
    /// \return The IP address.
    ////////////////////////////////////////////////////////////////////////////////////////////
    SockAddrIn GetClientIP();

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Get the client socket handle.
    /// \return a handle. Handles are 32-bit to allow compatibility between 32 and 64 bit
    ////////////////////////////////////////////////////////////////////////////////////////////
    CommunicationID GetClientHandle();

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Get the port that this object is using.
    /// \return the port beign used
    ////////////////////////////////////////////////////////////////////////////////////////////
    DWORD GetPort();

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Set the URL member.
    /// \param pURL The input URL string.
    ////////////////////////////////////////////////////////////////////////////////////////////
    void SetUrl(char* pURL);

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Get the URL string
    /// \return The URL string.
    ////////////////////////////////////////////////////////////////////////////////////////////
    char* GetUrl();

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Set the size of the posts data. This includes the name of the data, the '=' and the data itself in one string.
    /// \param nSize The size of the POST data.
    ////////////////////////////////////////////////////////////////////////////////////////////
    void SetPostDataSize(unsigned int nSize);

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets the size of the post data.
    /// \return The size in bytes.
    ////////////////////////////////////////////////////////////////////////////////////////////
    unsigned int GetPostDataSize();

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Set the post data
    /// \param pData The input data to copy.
    ////////////////////////////////////////////////////////////////////////////////////////////
    void SetPostData(char* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets the post data.
    /// \return A pointer to the post data.
    ////////////////////////////////////////////////////////////////////////////////////////////
    char* GetPostData();

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Get the shader code within the POST data.
    /// \return Pointer to the shader coide.
    ////////////////////////////////////////////////////////////////////////////////////////////
    char* GetShader();

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets the method string.
    /// \return The method string.
    ////////////////////////////////////////////////////////////////////////////////////////////
    char* GetMethod();

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets a pointer to the header data.
    /// \return A pointer to the header data.
    ////////////////////////////////////////////////////////////////////////////////////////////
    HTTPHeaderData* GetHeaderData();

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Reads and parses the HTTP hedaer data from the SOCKET
    /// \param strError Output error string.
    /// \param pClientSocket the socket to read
    /// \return True if success, false if fail.
    ////////////////////////////////////////////////////////////////////////////////////////////
    HTTP_REQUEST_RESULT ReadWebRequest(std::string& strError, NetSocket* pClientSocket);

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Read the POST data section of a web request from shared memory
    /// Both input streams read pointers must be set at the beginning of the POST data.
    /// \param strError Output error string
    /// \param pSharedMemoryName name of the shared memory to read
    /// \return True if success, False if fail.
    ////////////////////////////////////////////////////////////////////////////////////////////
    bool ReadPostData(std::string& strError, const char* pSharedMemoryName);

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Read the POST data section of a web request from a socket
    /// Both input streams read pointers must be set at the beginning of the POST data.
    /// \param strError Output error string
    /// \param pClientSocket the socket to read
    /// \return True if success, False if fail.
    ////////////////////////////////////////////////////////////////////////////////////////////
    bool ReadPostData(std::string& strError, NetSocket* pClientSocket);

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Get the content length of the POST data
    /// \param pBuffer THe buffer to search in.
    /// \return The content length of the POST data
    ////////////////////////////////////////////////////////////////////////////////////////////
    int GetContentLength(char* pBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Get the the ProtoInfo
    /// \return the proto info
    ////////////////////////////////////////////////////////////////////////////////////////////
    NetSocket::DuplicationInfo* GetProtoInfo() { return &m_httpHeaderData.ProtoInfo; }

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Checks to see if the process ID referenced in the request is still runnning
    /// \return True if running, false if not.
    ////////////////////////////////////////////////////////////////////////////////////////////
    bool CheckProcessStillRunning();

private:

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Preamble to reading the POST data section of a web request.
    /// \param strError Output error string
    /// \return length of data to read or 0 if error.
    ////////////////////////////////////////////////////////////////////////////////////////////
    unsigned int StartReadPostData(std::string& strError);

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Initiaize the header data.
    ////////////////////////////////////////////////////////////////////////////////////////////
    void InitializeHeaderData();

    ///////////////////////////////////////////////////////////////////////////////////////
    /// Read data from the input socket
    /// \param client_socket The socket connection.
    /// \param receiveBuffer The buffer to read the data into.
    /// \param bytesToRead The size of the output buffer.
    /// \return The number of bytes read.
    ///////////////////////////////////////////////////////////////////////////////////////
    static gtSize_t SocketRead(NetSocket* client_socket, char* receiveBuffer, gtSize_t bytesToRead);

    ///////////////////////////////////////////////////////////////////////////////////////
    /// Read just the web header
    /// \param client_socket The socket connection.
    /// \param pOutBuffer The buffer to read the data into.
    /// \param nBufferSize The size of the output buffer.
    /// \return The number of bytes read.
    ///////////////////////////////////////////////////////////////////////////////////////
    gtSize_t SocketReadHeader(NetSocket* client_socket, char* pOutBuffer, gtSize_t nBufferSize);

    ////////////////////////////////////////////////////////////////////////////////////////////
    /// Extract the data fields from the header buffer and populate the member data fields.
    /// \param pReceiveBuffer The input data
    /// \return True if suucess, False if fail.
    ////////////////////////////////////////////////////////////////////////////////////////////
    bool ExtractHeaderData(char* pReceiveBuffer);

private:

    /// Store the header data.
    HTTPHeaderData m_httpHeaderData;

    /// Pointer to the data passed in POST.
    char* m_pPostData;
};

#endif // GPS_HTTPREQUEST_H
