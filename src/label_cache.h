#include "SDL_ttf.h"
#include "SDL.h"

#include <unordered_map>
#include <cassert>
#include <algorithm>
#include <cstdint>

using u32 = uint32_t;

template<bool USE_SURFACE>
class LabelCache
{
public:
  using map_t = std::unordered_map<std::string, SDL_Rect>;
  using standalone_key_t = u32;
  struct standalone_cache_entry_t { std::string text; SDL_Rect rect; SDL_Texture* texture; };
  using standalone_map_t = std::unordered_map<standalone_key_t, standalone_cache_entry_t>;

private:
  int _w, _h;

  int _currentY, _currentX;
  int _maxY;

  TTF_Font* _font;
  SDL_Texture* _texture;
  SDL_Surface* _surface;
  SDL_Renderer* _renderer;
  map_t _cache;
  standalone_map_t _standaloneCache;

  map_t::const_iterator compute(const std::string& text)
  {
    SDL_Surface* surface = TTF_RenderUTF8_Blended(_font, text.c_str(), { 0, 0, 0, 255 });

    /* if it doesn't fit current row start a new one */
    if (_currentX + surface->w > _w)
    {
      _currentY = _maxY;
      _currentX = 0;
    }

    if (surface->w > _w)
      assert(false);

    int newY = _currentY + surface->h;

    /* if there is not enough vertical space there's no way */
    if (newY > _h)
      assert(false);

    /* update new column max */
    _maxY = std::max(_maxY, newY);

    auto pair = _cache.emplace(std::make_pair(text, SDL_Rect{ _currentX, _currentY, surface->w, surface->h }));

    _currentX += surface->w;

    if (USE_SURFACE)
    {
      SDL_BlitSurface(surface, nullptr, _surface, &pair.first->second);

      if (_texture)
        SDL_DestroyTexture(_texture);

      _texture = SDL_CreateTextureFromSurface(_renderer, _surface);
    }
    else
    {
      u32 format;
      int access, w, h;
      SDL_QueryTexture(_texture, &format, &access, &w, &h);
      assert(surface->format->format == format);

      SDL_Texture* texture = SDL_CreateTextureFromSurface(_renderer, surface);

      SDL_Rect src = { 0, 0, surface->w, surface->h };
      const SDL_Rect& dest = pair.first->second;

      SDL_SetRenderTarget(_renderer, _texture);
      SDL_RenderCopy(_renderer, texture, &src, &dest);
      SDL_SetRenderTarget(_renderer, nullptr);

      SDL_DestroyTexture(texture);
    }

    SDL_FreeSurface(surface);
      
    return pair.first;
  }

  std::pair<SDL_Rect, SDL_Texture*> computeStandalone(const std::string& text)
  {
    SDL_Surface* surface = TTF_RenderUTF8_Blended(_font, text.c_str(), { 0, 0, 0, 255 });
    SDL_Texture* texture = SDL_CreateTextureFromSurface(_renderer, surface);
    SDL_FreeSurface(surface);

    Uint32 format;
    int access, w, h;
    SDL_QueryTexture(texture, &format, &access, &w, &h);
  
    return std::make_pair(SDL_Rect{ 0, 0, w, h }, texture);
  }

public:
  LabelCache() : _font(nullptr), _texture(nullptr), _w(0), _h(0), _maxY(0), _currentY(0), _currentX(0)
  {

  }

  void init(TTF_Font* font, SDL_Renderer* renderer, int w, int h)
  {
    _font = font;
    //_renderer = renderer;
    _w = w;
    _h = h;

    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);


    _renderer = renderer;

    if (USE_SURFACE)
    {
      _surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
      SDL_FillRect(_surface, nullptr, 0);
    }
    else
    {
      _texture = SDL_CreateTexture(renderer, info.texture_formats[0], SDL_TEXTUREACCESS_TARGET, w, h);
      SDL_SetTextureBlendMode(_texture, SDL_BLENDMODE_BLEND);
      SDL_SetRenderTarget(_renderer, _texture);
      SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 0);
      SDL_RenderClear(_renderer);
      SDL_SetRenderTarget(_renderer, nullptr);
    }
  }

  typename standalone_map_t::const_iterator get(const std::string& text, standalone_key_t key)
  {
    typename standalone_map_t::iterator it = _standaloneCache.find(key);

    if (it == _standaloneCache.end())
    {
      auto texture = computeStandalone(text);
      return _standaloneCache.emplace(std::make_pair(key, standalone_cache_entry_t{ text, texture.first, texture.second })).first;
    }
    else if (text != it->second.text)
    {
      SDL_DestroyTexture(it->second.texture);
      auto texture = computeStandalone(text);
      it->second = { text, texture.first, texture.second };
      return it;
    }
    else
      return it;
  }


  map_t::const_iterator get(const std::string& text)
  {
    map_t::const_iterator it = _cache.find(text);

    if (it == _cache.end())
      it = compute(text);

    return it;
  }

  SDL_Texture* texture() const { return _texture; }

  ~LabelCache()
  {
    SDL_DestroyTexture(_texture);

    if (USE_SURFACE)
      SDL_FreeSurface(_surface);
  }
};