# Step 5: Drawing Text

This step adds text rendering using `SpriteFont` and `SpriteBatch`. See the wiki page: [Drawing text](https://github.com/microsoft/DirectXTK12/wiki/Drawing-text).

## Setup

Copy the `courier.spritefont` file from this skill's `assets/` folder into the project's working directory:

```powershell
Copy-Item "<SkillDir>\assets\courier.spritefont" -Destination "<ProjectDir>"
```

> For details on how the sprite font is created, see the [MakeSpriteFont](https://github.com/microsoft/DirectXTK/wiki/MakeSpriteFont) wiki page.

## Code Changes

### pch.h

Add the following include to `pch.h` (after the existing DirectXTK12 includes):

```cpp
#include <directxtk12/SpriteFont.h>
```

### Game.h

Add the following private member variable to the `Game` class:

```cpp
std::unique_ptr<DirectX::SpriteFont> m_font;
```

Also update the `Descriptors` enum to add a slot for the font texture:

```cpp
enum Descriptors
{
    Cat,
    MyFont,
    Count
};
```

### Game.cpp — CreateDeviceDependentResources

In `CreateDeviceDependentResources`, load the sprite font inside the `ResourceUploadBatch` Begin/End block (after `resourceUpload.Begin()` and before `resourceUpload.End()`):

```cpp
m_font = std::make_unique<SpriteFont>(device, resourceUpload,
    L"courier.spritefont",
    m_resourceDescriptors->GetCpuHandle(Descriptors::MyFont),
    m_resourceDescriptors->GetGpuHandle(Descriptors::MyFont));
```

### Game.cpp — Render

In the `Render` method, inside the existing `SpriteBatch` Begin/End block (after the sprite draw), add text rendering:

```cpp
ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptors->Heap() };
commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

m_spriteBatch->Begin(commandList);

m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Cat),
    GetTextureSize(m_texture.Get()),
    m_screenPos, nullptr, Colors::White, 0.f, m_origin);

m_font->DrawString(m_spriteBatch.get(), L"Hello, World!",
    XMFLOAT2(10.f, 10.f), Colors::Yellow);

m_spriteBatch->End();
```

### Game.cpp — OnDeviceLost

In `OnDeviceLost`, release the font:

```cpp
m_font.reset();
```

## Build and Verify

Build the project using the commands from [Step 2](step2.md). Run the application from the project folder (where `courier.spritefont` is located).

You should see "Hello, World!" rendered in yellow text in the top-left corner of the window, with the cat sprite still centered.

## Further Reading

- [Drawing text](https://github.com/microsoft/DirectXTK12/wiki/Drawing-text)
  - Creating a font
  - Using std::wstring for text
  - Using ASCII text
  - Drop-shadow effect
  - Outline effect
  - More to explore

## Technical Notes

**Ask the user:** Would you like to see the Technical Notes for this step, or skip to the next step?

### SpriteFont

`SpriteFont` is a text rendering class that works in conjunction with `SpriteBatch`. In Direct3D 12, it manages:

- **Font atlas** — A single **`ID3D12Resource`** (committed resource in the default heap) containing all pre-rendered glyphs packed into a texture atlas. The shader resource view is stored in the descriptor heap at the handle provided during construction.
- **Glyph data** — A sorted array of `Glyph` structs (loaded from the `.spritefont` binary format) containing character code, source rectangle in the atlas, and advance/offset metrics.

**Text rendering pipeline:**

1. `DrawString` iterates the input text character by character via an internal `ForEachGlyph` helper.
2. For each character, it performs a binary search to find the corresponding `Glyph` (falling back to a default character if not found).
3. It computes the destination position using the glyph offset, advance width, and any rotation/scale/origin transforms.
4. It calls `SpriteBatch::Draw` for each glyph, passing the font atlas GPU descriptor handle and the glyph's source rectangle.
5. `SpriteBatch` batches all glyph quads together (since they share the same atlas texture) and renders them in a single `DrawIndexedInstanced` call.

The `.spritefont` binary format is produced by the **MakeSpriteFont** tool, which rasterizes a TrueType/OpenType font at a specific size and packs the glyphs into an atlas.

### Descriptor Heap Management

Note that both the cat texture and the font texture share the same `DescriptorHeap`. In Direct3D 12, you can only have one CBV/SRV/UAV descriptor heap and one sampler descriptor heap active at a time (set via `SetDescriptorHeaps`). By putting all texture descriptors in a single heap, we avoid the overhead of switching heaps during rendering.
