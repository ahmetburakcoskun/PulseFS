ID3D11ShaderResourceView* IconCache::GetImageThumbnail(const std::wstring& path) {
  if (!m_device) {
    return nullptr;
  }

  auto it = m_thumbnailCache.find(path);
  if (it != m_thumbnailCache.end()) {
    return it->second.Get();
  }

  CoInitialize(nullptr);

  Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory;
  HRESULT hr = CoCreateInstance(
    CLSID_WICImagingFactory,
    nullptr,
    CLSCTX_INPROC_SERVER,
    IID_PPV_ARGS(&wicFactory)
  );

  if (FAILED(hr)) {
    return nullptr;
  }

  Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
  hr = wicFactory->CreateDecoderFromFilename(
    path.c_str(),
    nullptr,
    GENERIC_READ,
    WICDecodeMetadataCacheOnDemand,
    &decoder
  );

  if (FAILED(hr)) {
    return nullptr;
  }

  Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
  hr = decoder->GetFrame(0, &frame);
  if (FAILED(hr)) {
    return nullptr;
  }

  UINT width = 0, height = 0;
  frame->GetSize(&width, &height);

  const UINT maxSize = 256;
  UINT thumbWidth = width;
  UINT thumbHeight = height;
  
  if (width > maxSize || height > maxSize) {
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    if (width > height) {
      thumbWidth = maxSize;
      thumbHeight = static_cast<UINT>(maxSize / aspect);
    } else {
      thumbHeight = maxSize;
      thumbWidth = static_cast<UINT>(maxSize * aspect);
    }
  }

  Microsoft::WRL::ComPtr<IWICBitmapScaler> scaler;
  hr = wicFactory->CreateBitmapScaler(&scaler);
  if (FAILED(hr)) {
    return nullptr;
  }

  hr = scaler->Initialize(frame.Get(), thumbWidth, thumbHeight, WICBitmapInterpolationModeFant);
  if (FAILED(hr)) {
    return nullptr;
  }

  Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
  hr = wicFactory->CreateFormatConverter(&converter);
  if (FAILED(hr)) {
    return nullptr;
  }

  hr = converter->Initialize(
    scaler.Get(),
    GUID_WICPixelFormat32bppBGRA,
    WICBitmapDitherTypeNone,
    nullptr,
    0.0f,
    WICBitmapPaletteTypeCustom
  );

  if (FAILED(hr)) {
    return nullptr;
  }

  UINT stride = thumbWidth * 4;
  UINT bufferSize = stride * thumbHeight;
  std::vector<BYTE> buffer(bufferSize);

  hr = converter->CopyPixels(nullptr, stride, bufferSize, buffer.data());
  if (FAILED(hr)) {
    return nullptr;
  }

  D3D11_TEXTURE2D_DESC texDesc = {};
  texDesc.Width = thumbWidth;
  texDesc.Height = thumbHeight;
  texDesc.MipLevels = 1;
  texDesc.ArraySize = 1;
  texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  texDesc.SampleDesc.Count = 1;
  texDesc.Usage = D3D11_USAGE_DEFAULT;
  texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

  D3D11_SUBRESOURCE_DATA initData = {};
  initData.pSysMem = buffer.data();
  initData.SysMemPitch = stride;

  Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
  hr = m_device->CreateTexture2D(&texDesc, &initData, &texture);
  if (FAILED(hr)) {
    return nullptr;
  }

  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format = texDesc.Format;
  srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = 1;

  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
  hr = m_device->CreateShaderResourceView(texture.Get(), &srvDesc, &srv);
  if (FAILED(hr)) {
    return nullptr;
  }

  m_thumbnailCache[path] = srv;
  return srv.Get();
}// namespace PulseFS::UI
