#include "AtmosphereModel.h"

NS_RENDERSYSTEM_BEGIN

// utils begin

static double Interpolate(
    const std::vector<double>& wavelengths,
    const std::vector<double>& wavelength_function,
    double wavelength)
{
    assert(wavelength_function.size() == wavelengths.size());
    if (wavelength < wavelengths[0])
    {
        return wavelength_function[0];
    }
    for (unsigned int i = 0; i < wavelengths.size() - 1; ++i)
    {
        if (wavelength < wavelengths[i + 1])
        {
            double u = (wavelength - wavelengths[i]) / (wavelengths[i + 1] - wavelengths[i]);
            return wavelength_function[i] * (1.0 - u) + wavelength_function[i + 1] * u;
        }
    }
    return wavelength_function[wavelength_function.size() - 1];
}

static Vector3f GetInterpolateValue(const std::vector<double>& wavelengths, const std::vector<double> &v,
                                    const Vector3f& lambdas, double scale)
{
    double r = Interpolate(wavelengths, v, lambdas[0]) * scale;
    double g = Interpolate(wavelengths, v, lambdas[1]) * scale;
    double b = Interpolate(wavelengths, v, lambdas[2]) * scale;
    
    return Vector3f(r, g, b);
}

static void FillDensityProfile(std::vector<DensityProfileLayer> layers, Atmosphere::DensityProfile & densityProfile)
{
    constexpr int kLayerCount = 2;
    while (layers.size() < kLayerCount) 
    {
        layers.insert(layers.begin(), DensityProfileLayer());
    }
    
    for (int i = 0; i < kLayerCount; ++i) 
    {
        densityProfile.layers[i].width = layers[i].width;
        densityProfile.layers[i].constant_term = layers[i].constant_term;
        densityProfile.layers[i].exp_scale = layers[i].exp_scale;
        densityProfile.layers[i].exp_term = layers[i].exp_term;
        densityProfile.layers[i].linear_term = layers[i].linear_term;
    }
};

// utils end

AtmosphereModel::AtmosphereModel(
        const std::vector<double> &wavelengths,
        const std::vector<double> &solar_irradiance,
        double sun_angular_radius,
        double bottom_radius,
        double top_radius,
        const std::vector<DensityProfileLayer> &rayleigh_density,
        const std::vector<double> &rayleigh_scattering,
        const std::vector<DensityProfileLayer> &mie_density,
        const std::vector<double> &mie_scattering,
        const std::vector<double> &mie_extinction,
        double mie_phase_function_g,
        const std::vector<DensityProfileLayer> &absorption_density,
        const std::vector<double> &absorption_extinction,
        const std::vector<double> &ground_albedo,
        double max_sun_zenith_angle,
        double length_unit_in_meters,
        bool combine_scattering_textures,
        bool half_precision)
{    
    Vector3f lambdas = Vector3f(Atmosphere::kLambdaR, Atmosphere::kLambdaG, Atmosphere::kLambdaB);
    mAtomSphere.solar_irradiance = GetInterpolateValue(wavelengths, solar_irradiance, lambdas, 1.0);
    mAtomSphere.sun_angular_radius = sun_angular_radius;
    mAtomSphere.bottom_radius = bottom_radius / length_unit_in_meters;
    mAtomSphere.top_radius = top_radius / length_unit_in_meters;
    FillDensityProfile(rayleigh_density, mAtomSphere.rayleigh_density);
    mAtomSphere.rayleigh_scattering = GetInterpolateValue(wavelengths, rayleigh_scattering, lambdas, length_unit_in_meters);
    FillDensityProfile(mie_density, mAtomSphere.mie_density);
    mAtomSphere.mie_scattering = GetInterpolateValue(wavelengths, mie_scattering, lambdas, length_unit_in_meters);
    mAtomSphere.mie_extinction = GetInterpolateValue(wavelengths, mie_extinction, lambdas, length_unit_in_meters);
    mAtomSphere.mie_phase_function_g = mie_phase_function_g;
    FillDensityProfile(absorption_density, mAtomSphere.absorption_density);
    mAtomSphere.absorption_extinction = GetInterpolateValue(wavelengths, absorption_extinction, lambdas, length_unit_in_meters);
    mAtomSphere.ground_albedo = GetInterpolateValue(wavelengths, ground_albedo, lambdas, length_unit_in_meters);
    mAtomSphere.mu_s_min = cos(max_sun_zenith_angle);
    
}

NS_RENDERSYSTEM_END
