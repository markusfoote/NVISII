
#pragma once

#include <nvisii/utilities/static_factory.h>
#include <nvisii/light_struct.h>

namespace nvisii {

class Texture;

/**
 * A "Light" component illuminates objects in a scene. Light components must 
 * be added to an entity with a transform component to have a visible      
 * impact on the scene.                                                    
*/
class Light : public StaticFactory {
    friend class StaticFactory;
    friend class Entity;
public:
    /**
      * Instantiates a null Light. Used to mark a row in the table as null. 
      * Note: for internal use only. 
     */
    Light();
    
    /**
      * Instantiates a Light with the given name and ID. Used to mark a row in 
      * the table as null. 
      * Note: for internal use only.
    */
    Light(std::string name, uint32_t id);

    /** 
     * Constructs a light with the given name.
     * 
     * @param name A unique name for this light
     * @returns a reference to a light component
     */
    static Light* create(std::string name);

    /** 
     * Constructs a light component which emits a plausible light color based on 
     * standard temperature measurement. 
     * 
     * @param name A unique name for this light
     * @param kelvin The temperature of the black body light. 
     * Typical values range from 1000K (very warm) to 12000K (very cold).
     * @param intensity How powerful the light source is in emitting light
     * @returns a reference to a light component
    */
    static Light* createFromTemperature(std::string name, float kelvin, float intensity);
    
    /** 
     * Constructs a light component which emits a given light color.
     *
     * @param name A unique name for this light.
     * @param color An RGB color to emit. Values should range between 0 and 1. 
     * @param intensity How powerful the light source is in emitting light 
    */
    static Light* createFromRGB(std::string name, glm::vec3 color, float intensity);

    /** 
     * @param name The name of the light to get
     * @returns a Light who's name matches the given name 
    */
    static Light* get(std::string name);

    /** @returns a pointer to the table of LightStructs required for rendering */
    static LightStruct* getFrontStruct();

    /** @returns a pointer to the table of light components */
    static Light* getFront();

    /** @returns the number of allocated lights */
    static uint32_t getCount();

    /** @returns the name of this component */
	std::string getName();

    /** @returns A map whose key is a light name and whose value is the ID for 
     * that light */
	static std::map<std::string, uint32_t> getNameToIdMap();

    /** @param name The name of the Light to remove */
    static void remove(std::string name);

    /** Allocates the tables used to store all light components */
    static void initializeFactory(uint32_t max_components);

    /** @return True if the tables used to store all light components have been 
     * allocated, and False otherwise */
    static bool isFactoryInitialized();
    
    /** @return True the current light is a valid, initialized light, and False 
     * if the light was cleared or removed. */
	bool isInitialized();

    /** Iterates through all light components, computing light metadata for 
     * rendering purposes. */
    static void updateComponents();

    /** Clears any existing light components. */
    static void clearAll();

    /** @returns a json string representation of the current component */
    std::string toString();

    /** @return True if any the light has been modified since the previous frame, 
     * and False otherwise */
    static bool areAnyDirty();

    /** @returns True if this lightmaterial has been modified since the previous 
     * frame, and False otherwise */
    bool isDirty() { return dirty; }

    /** @returns True if the light has not been modified since the previous frame, 
     * and False otherwise */
    bool isClean() { return !dirty; }

    /** Tags the current component as being modified since the previous frame. */
    void markDirty();

    /** Returns the simplified struct used to represent the current component */
    LightStruct &getStruct();

    /** Tags the current component as being unmodified since the previous frame. */
    void markClean() { dirty = false; }

    /** For internal use. Returns the mutex used to lock entities for processing 
     * by the renderer. */
    static std::shared_ptr<std::recursive_mutex> getEditMutex();

    /** 
     * Sets the color which this light component will emit. 
     *
     * @param The RGB color emitted that this light should emit.
    */
    void setColor(glm::vec3 color);

    /** 
     * Sets the color which this light component will emit. Texture is expected 
     * to be RGB. Overrides any existing constant light color. 
     *
     * @param texture An RGB texture component whose values range between 0 and 1. 
     * Alpha channel is ignored.
    */
    void setColorTexture(Texture *texture);

    /** Disconnects the color texture, reverting back to any existing constant 
     * light color*/
    void clearColorTexture();

    /** @returns the constant vec3 color used by this light. If a color texture 
     * is set, this function should not be used. */
    glm::vec3 getColor();
    
    /** 
     * Sets a realistic emission color via a temperature.
     *
     * @param kelvin The temperature of the black body light. Typical values 
     * range from 1000K (very warm) to 12000K (very cold).
    */
    void setTemperature(float kelvin);

    /** 
     * Sets the intensity, or brightness, that this light component will emit 
     * it's color. 
     * 
     * @param intensity How powerful the light source is in emitting light 
    */
    void setIntensity(float intensity);

    /** @returns the constant intensity used by this light. */
    float getIntensity();

    /** 
     * Modifies the intensity, or brightness, that this light component will emit 
     * it's color by a power of 2.
     * Increasing the exposure by 1 will double the energy emitted by the light. 
     * An exposure of 0 produces an unmodified intensity.
     * An exposure of -1 cuts the intensity of the light in half.
     * light_intensity = intensity * pow(2, exposureExposure)
     * 
     * @param exposure How powerful the light source is in emitting light.
    */
    void setExposure(float exposure);

    /** @returns the constant exposure used by this light. */
    float getExposure();

    /** 
     * Modifies the falloff exponent that this light component will use to reduce 
     * intensity due to distance. Physically realistic light transport uses an 
     * r^2 falloff, where the falloff exponent equals 2. Many video games instead
     * use a linear falloff (falloff = 1). Distance falloff can be disabled 
     * in its entirety by setting falloff to 0.
     * 
     * @param falloff The distance falloff exponent to use. 
    */
    void setFalloff(float falloff);

    /** @returns the distance falloff exponent used by this light. */
    float getFalloff();

    /**
     * Controls whether or not the surface area of the light should effect 
     * overall light intensity.
     * @param use if True, allows the area of the light to affect intensity.
    */
    void useSurfaceArea(bool use);
    
private:

    /* A mutex used to make component access and modification thread safe */
    static std::shared_ptr<std::recursive_mutex> editMutex;

    /* Flag indicating that static resources were created */
    static bool factoryInitialized;

    /* A list of light components, allocated statically */
    static std::vector<Light> lights;
    static std::vector<LightStruct> lightStructs;

    /* A lookup table of name to light id */
    static std::map<std::string, uint32_t> lookupTable;

    /* Indicates that one of the components has been edited */
    static bool anyDirty;

    /* Indicates this component has been edited */
    bool dirty = true;
};

};
