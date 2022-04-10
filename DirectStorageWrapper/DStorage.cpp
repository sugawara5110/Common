//*****************************************************************************************//
//**                                                                                     **//
//**                   　　　         DStorage                                           **//
//**                                                                                     **//
//*****************************************************************************************//

#define _CRT_SECURE_NO_WARNINGS
#include "DStorage.h"
#include <assert.h>
#include <tchar.h>
#include <locale.h>
#include "../../PNGLoader/PNGLoader.h"
#include "../../JPGLoader/JPGLoader.h"

namespace {

	struct handle_closer
	{
		void operator()(HANDLE h) noexcept
		{
			assert(h != INVALID_HANDLE_VALUE);
			if (h)
			{
				CloseHandle(h);
			}
		}
	};

	using ScopedHandle = std::unique_ptr<void, handle_closer>;

	ComPtr<IDStorageFactory> factory;
	ComPtr<IDStorageCustomDecompressionQueue> g_customDecompressionQueue;
	HANDLE g_customDecompressionQueueEvent;
	TP_WAIT* g_threadpoolWait;

	InputParameter* inCopy = nullptr;

	std::unique_ptr<OutputResource[]> OutResource = nullptr;

	struct DSTORAGE_CUSTOM_DECOMPRESSION_REQUEST_AND_INDEX {
		DSTORAGE_CUSTOM_DECOMPRESSION_REQUEST req = {};
		int index = 0;
	};

	int indexCnt = 0;

	void CALLBACK Decompress(
		TP_CALLBACK_INSTANCE*,
		void* context) {

		auto* re = reinterpret_cast<DSTORAGE_CUSTOM_DECOMPRESSION_REQUEST_AND_INDEX*>(context);

		PNGLoader png;
		JPGLoader jpg;
		UINT SrcSize = (UINT)re->req.SrcSize;
		UINT DstSize = (UINT)re->req.DstSize;
		unsigned char* src = (unsigned char*)re->req.SrcBuffer;
		unsigned char* dst = (unsigned char*)re->req.DstBuffer;
		UINT wid = inCopy[re->index].width;
		UINT hei = inCopy[re->index].height;

		UCHAR* byte = png.loadPngInByteArray(src, SrcSize, wid, hei);
		if (!byte)byte = jpg.loadJpgInByteArray(src, SrcSize, wid, hei);
		OutResource[re->index].Subresource = byte;

		memcpy(dst, byte, DstSize);

		DSTORAGE_CUSTOM_DECOMPRESSION_RESULT result{};
		result.Id = re->req.Id;
		result.Result = S_OK;

		g_customDecompressionQueue->SetRequestResults(1, &result);

		delete re;
	}

	void TrySubmitThread(DSTORAGE_CUSTOM_DECOMPRESSION_REQUEST_AND_INDEX* req) {
		DSTORAGE_CUSTOM_DECOMPRESSION_REQUEST_AND_INDEX* dst = new DSTORAGE_CUSTOM_DECOMPRESSION_REQUEST_AND_INDEX();
		*dst = *req;
		TrySubmitThreadpoolCallback(Decompress, dst, nullptr);
	}

	void CALLBACK OnCustomDecompressionRequest(
		TP_CALLBACK_INSTANCE*,
		void*,
		TP_WAIT* wait,
		TP_WAIT_RESULT) {

		DSTORAGE_CUSTOM_DECOMPRESSION_REQUEST request[64];
		while (true) {

			uint32_t numRequests = 0;
			g_customDecompressionQueue->GetRequests(_countof(request), request, &numRequests);
			if (numRequests <= 0) {
				SetThreadpoolWait(wait, g_customDecompressionQueueEvent, nullptr);
				break;
			}
			else {
				for (uint32_t i = 0; i < numRequests; i++) {
					DSTORAGE_CUSTOM_DECOMPRESSION_REQUEST_AND_INDEX ri = {};
					ri.req = request[i];
					ri.index = indexCnt++;
					TrySubmitThread(&ri);
				}
			}
		}
	}

	void setEr(char* errorMessage, char* inMessage) {
		if (errorMessage) {
			int size = (int)strlen(inMessage) + 1;
			memcpy(errorMessage, inMessage, size);
		}
	}

	wchar_t* charToWchar(char* str) {
		//ロケール指定
		setlocale(LC_ALL, "japanese");
		static wchar_t buf[256] = {};
		buf[255] = '\0';
		//char→wchar_t変換
		int size = (int)strlen(str) + 1;
		if (size > 255)size = 255;
		mbstowcs(buf, str, size);
		return buf;
	}
}

void DStorage::Delete() {
	if (!factory)
		return;

	CloseThreadpoolWait(g_threadpoolWait);
	g_customDecompressionQueue.Reset();
	CloseHandle(g_customDecompressionQueueEvent);
	factory.Reset();
	indexCnt = 0;
}

std::unique_ptr<OutputResource[]> DStorage::Load(
	ID3D12Device* device,
	InputParameter* in, int numArr,
	char* error) {

	inCopy = in;

	if (FAILED(DStorageGetFactory(IID_PPV_ARGS(factory.GetAddressOf())))) {
		if (error) {
			setEr(error, "DStorageGetFactory Error");
		}
		return nullptr;
	}

	std::unique_ptr<ComPtr<IDStorageFile>[]> file = std::make_unique<ComPtr<IDStorageFile>[]>(numArr);
	for (int i = 0; i < numArr; i++) {
		if (FAILED(factory->OpenFile(charToWchar(in[i].getFileName()), IID_PPV_ARGS(file[i].GetAddressOf())))) {
			if (error) {
				setEr(error, "OpenFile Error");
			}
			return nullptr;
		}
	}

	DSTORAGE_QUEUE_DESC queueDesc{};
	queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
	queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
	queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
	queueDesc.Device = device;

	ComPtr<IDStorageQueue> queue;
	if (FAILED(factory->CreateQueue(&queueDesc, IID_PPV_ARGS(queue.GetAddressOf())))) {
		if (error) {
			setEr(error, "CreateQueue Error");
		}
		return nullptr;
	}

	factory.As(&g_customDecompressionQueue);
	g_customDecompressionQueueEvent = g_customDecompressionQueue->GetEvent();
	g_threadpoolWait = CreateThreadpoolWait(OnCustomDecompressionRequest, nullptr, nullptr);
	SetThreadpoolWait(g_threadpoolWait, g_customDecompressionQueueEvent, nullptr);

	OutResource = std::make_unique<OutputResource[]>(numArr);

	for (int i = 0; i < numArr; i++) {

		D3D12_HEAP_PROPERTIES bufferHeapProps = {};
		bufferHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		bufferDesc.Width = in[i].width;
		bufferDesc.Height = in[i].height;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = in[i].format;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;

		if (FAILED(device->CreateCommittedResource(
			&bufferHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(OutResource[i].Texture.GetAddressOf())))) {
			if (error) {
				setEr(error, "CreateCommittedResource Error");
			}
			return nullptr;
		}

		BY_HANDLE_FILE_INFORMATION info{};
		if (FAILED(file[i]->GetFileInformation(&info))) {
			if (error) {
				setEr(error, "GetFileInformation Error");
			}
			return nullptr;
		}
		uint32_t fileSize = info.nFileSizeLow;

		DSTORAGE_REQUEST request = {};
		request.Options.CompressionFormat = DSTORAGE_CUSTOM_COMPRESSION_0;
		request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
		request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION;
		request.Source.File.Source = file[i].Get();
		request.Source.File.Offset = 0;
		request.Source.File.Size = fileSize;
		request.UncompressedSize = in[i].width * in[i].height * 4;
		request.Destination.Texture.Resource = OutResource[i].Texture.Get();
		request.Destination.Texture.SubresourceIndex = 0;
		D3D12_BOX box = {};
		box.right = in[i].width;
		box.bottom = in[i].height;
		box.back = 1;
		request.Destination.Texture.Region = box;

		queue->EnqueueRequest(&request);
	}

	ComPtr<ID3D12Fence> fence;
	if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())))) {
		if (error) {
			setEr(error, "CreateFence Error");
		}
		return nullptr;
	}

	ScopedHandle fenceEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr));
	constexpr uint64_t fenceValue = 1;

	if (FAILED(fence->SetEventOnCompletion(fenceValue, fenceEvent.get()))) {
		if (error) {
			setEr(error, "SetEventOnCompletion Error");
		}
		return nullptr;
	}

	queue->EnqueueSignal(fence.Get(), fenceValue);

	// 実行開始
	queue->Submit();

	WaitForSingleObject(fenceEvent.get(), INFINITE);

	DSTORAGE_ERROR_RECORD errorRecord{};
	queue->RetrieveErrorRecord(&errorRecord);
	if (FAILED(errorRecord.FirstFailure.HResult)) {
		if (error) {
			setEr(error, "RetrieveErrorRecord Error");
		}
		return nullptr;
	}

	setEr(error, "OK");
	return std::move(OutResource);
}