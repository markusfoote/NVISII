#pragma once

#include <mutex>
#include <condition_variable>

#include <visii/utilities/static_factory.h>
#include <visii/texture_struct.h>

/**
 * The "Texture" component describes a 2D pattern used to drive the "Material" component's parameters.
*/
class Texture : public StaticFactory
{
	friend class StaticFactory;
  public:
	
	/** Constructs a Texture with the given name.
     * \return a Texture allocated by the renderer. */
	static Texture *create(std::string name);

	/** Constructs a Texture with the given name from a image located on the filesystem. 
	 * Supported formats include JPEG, PNG, TGA, BMP, PSD, GIF, HDR, PIC, and PNM
	 * \param path The path to the image.
     * \return a Texture allocated by the renderer. */
	static Texture *createFromImage(std::string name, std::string path);

	/** Constructs a Texture with the given name from custom user data.
	 * \param width The width of the image.
	 * \param height The height of the image.
	 * \param data A row major flattened vector of RGBA texels.
     * \return a Texture allocated by the renderer. */
	static Texture *createFromData(std::string name, uint32_t width, uint32_t height, std::vector<glm::vec4> data);

    /** Gets a Texture by name 
     * \return a Texture who's primary name key matches \p name */
	static Texture *get(std::string name);

	/** Gets a Texture by id 
     * \return a Texture who's primary id key matches \p id */
	static Texture *get(uint32_t id);

    /** \return a pointer to the table of TextureStructs */
	static TextureStruct *getFrontStruct();

	/** \return a pointer to the table of Texture components */
	static Texture *getFront();

	/** \returns the number of allocated textures */
	static uint32_t getCount();

	/** Deletes the Texture who's primary name key matches \p name */
	static void remove(std::string name);

	/** Deletes the Texture who's primary id key matches \p id */
	static void remove(uint32_t id);

	/** Allocates the tables used to store all Texture components */
	static void initializeFactory();

	/** \return True if the tables used to store all Texture components have been allocated, and False otherwise */
	static bool isFactoryInitialized();
    
    /** \return True the current Texture is a valid, initialized Texture, and False if the Texture was cleared or removed. */
	bool isInitialized();

    static void updateComponents();

	/* Releases vulkan resources */
	static void cleanUp();

    /** \return True if the material has been modified since the previous frame, and False otherwise */
    bool isDirty() { return dirty; }

	static bool areAnyDirty();

    /** \return True if the material has not been modified since the previous frame, and False otherwise */
    bool isClean() { return !dirty; }

    /** Tags the current component as being modified since the previous frame. */
    void markDirty();

    /** Tags the current component as being unmodified since the previous frame. */
    void markClean() { dirty = false; }

	static std::shared_ptr<std::mutex> getEditMutex();

    /** \returns a json string representation of the current component */
    std::string toString();

    /** \returns a flattened list of texels */
    std::vector<vec4> getTexels();

    /** \returns the width of the texture in texels */
    uint32_t getWidth();
    
    /** \returns the height of the texture in texels */
    uint32_t getHeight();

  private:
  	/* Creates an uninitialized texture. Useful for preallocation. */
	Texture();

	/* Creates a texture with the given name and id. */
	Texture(std::string name, uint32_t id);

  	/* TODO */
	static std::shared_ptr<std::mutex> editMutex;

	/* TODO */
	static bool factoryInitialized;
	
    /* A list of the camera components, allocated statically */
	static Texture textures[MAX_TEXTURES];
	static TextureStruct textureStructs[MAX_TEXTURES];
	
	/* A lookup table of name to camera id */
	static std::map<std::string, uint32_t> lookupTable;

    /* Indicates that one of the components has been edited */
    static bool anyDirty;

    /* Indicates this component has been edited */
    bool dirty = true;

    /* The texels of the texture */
    std::vector<vec4> texels;
};