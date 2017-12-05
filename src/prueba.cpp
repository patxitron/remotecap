/*
 * Ejemplo de captura con la ASI1600 y creación de un fichero fits
 * 
 * Compilar con
 *      g++ -std=c++11 -o prueba prueba.cpp ../zwo/asi/lib/x64/libASICamera2.a -lcfitsio -lCCfits -lusb-1.0 -lpthread
 *
 */


#include <stdio.h>
#include <unistd.h>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <valarray>
#include <iostream>
#include <ctime>
#include <cstring>
#include <CCfits/CCfits>
#include "../zwo/asi/include/ASICamera2.h"


using namespace CCfits;
using namespace std;

typedef vector<string> vectorstring;



int drive_temp(int old_target_temp, int current_temp, int set_temp)
{
    cout << "drive_temp(" << old_target_temp << ", " << current_temp << ", " << set_temp << ")" << endl;
    if (current_temp <= old_target_temp && old_target_temp > set_temp) {
        old_target_temp -= 5;
        if (old_target_temp < set_temp) old_target_temp = set_temp;
        cout << "Changed set point to " << old_target_temp << endl;
    }
    return old_target_temp;
}



int writeImage(char const* filename, int width, int height, unsigned char* img_buf, double exp=0, double temp=0.0, int gain=0, int offset=0) {
    /**
     * Número de dimensiones de la imagen (2D)
     */
    long naxis = 2;
    /**
     * Vector con el tamaño de cada dimensión
     */
    long axes[2] = {width, height};
    /*
     * Creamos un objeto tipo FITS en un puntero único
     */
    std::unique_ptr<FITS> fits(nullptr);
    try {
        // overwrite existing file if the file already exists.
        std::string const file_name(filename);
        fits.reset(new FITS(file_name, USHORT_IMG, naxis, axes));
    } catch (FITS::CantCreate) {
        return -1;
    }
    /*
     * Construimos un valarray y copiamos en él la imagen.
     */
    long nelements = axes[0] * axes[1];
    std::valarray<int> array(nelements);
    for (int i = 0, j = 0; i < nelements; i++, j += 2) {
        array[i] = img_buf[j] + (img_buf[j + 1] << 8);
    }
    /*
     * Creamos varios "keyword" fits
     */
    fits->pHDU().addKey("EXPOSURE", exp, "Total Exposure Time");
    fits->pHDU().addKey("CCD-TEMP", temp, "Sensor temperature in celsius");
    fits->pHDU().addKey("CCD-GAIN", gain, "Sensor gain");
    fits->pHDU().addKey("CCD-OFFSET", offset, "Sensor offset");
    fits->pHDU().addKey("TIMESTAMP", time(nullptr), "Exposition timestamp in unix time");
    /*
     * Escribimos la imagen en el objeto fits
     */
    fits->pHDU().write(1, nelements, array);
    /*
     * Sacamos por consola información sobre el fits
     */
    std::cout << fits->pHDU() << std::endl;
    return 0;
}

int main(int argc, char* argv[])
{
    int num_devices = ASIGetNumOfConnectedCameras();
    int camera_number = 0;
    int exposure = 4000; // 4 seconds
    int gain = 0;
    int offset = 10;
    int set_temp = 30;
    size_t count = 1;
    string capture_name("capture");

    vectorstring args;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            args.emplace_back(argv[i]);
        }
    }

    vectorstring::iterator index(find(args.begin(), args.end(), "-n"));
    if (index != args.end() && ++index != args.end()) {
        capture_name = *index;
    }

    index = find(args.begin(), args.end(), "-e");
    if (index != args.end() && ++index != args.end()) {
        stringstream sindex(*index);
        sindex >> exposure;
        if (exposure <= 0) exposure = 4000;
    }

    index = find(args.begin(), args.end(), "-g");
    if (index != args.end() && ++index != args.end()) {
        stringstream sindex(*index);
        sindex >> gain;
        if (gain < 0) gain = 0;
        if (gain > 300) gain = 300;
    }

    index = find(args.begin(), args.end(), "-o");
    if (index != args.end() && ++index != args.end()) {
        stringstream sindex(*index);
        sindex >> offset;
        if (offset < 0) offset = 0;
        if (offset > 60) offset = 60;
    }

    index = find(args.begin(), args.end(), "-c");
    if (index != args.end() && ++index != args.end()) {
        stringstream sindex(*index);
        sindex >> count;
        if (count < 1) count = 1;
    }

    index = find(args.begin(), args.end(), "-t");
    if (index != args.end() && ++index != args.end()) {
        stringstream sindex(*index);
        sindex >> set_temp;
        if (set_temp < -40) set_temp = -40;
        if (set_temp > 30) set_temp = 30;
    }

    char const* bayer[] = {"RG","BG","GR","GB"};
    FITS::setVerboseMode(true);
    if (num_devices <= 0) {
        printf("no camera connected!\n");
        return -1;
    }

    ASI_CAMERA_INFO ASI_camera_info;

    if (num_devices > 1) {
        for (int i = 0; i < num_devices; i++) {
            printf("attached cameras:\n");
            ASIGetCameraProperty(&ASI_camera_info, i);
            printf("\t%d - '%s'\n",i, ASI_camera_info.Name);
        }
        printf("\nselect one to preview: ");
        scanf("%d", &camera_number);
    } else {
        ASIGetCameraProperty(&ASI_camera_info, 0);
        printf("attached camera: '%s'\n", ASI_camera_info.Name);
    }

    if (ASIOpenCamera(camera_number) != ASI_SUCCESS) {
        printf("OpenCamera error, are you root?\n");
        return -1;
    }
    ASIInitCamera(camera_number);

    printf("%s information:\n", ASI_camera_info.Name);
    int max_width, max_height;
    max_width = ASI_camera_info.MaxWidth;
    max_height =  ASI_camera_info.MaxHeight;
    printf("\tresolution:%dx%d\n", max_width, max_height);
    if (ASI_camera_info.IsColorCam) {
        printf("\tColor Camera: bayer pattern:%s\n", bayer[ASI_camera_info.BayerPattern]);
    } else {
        printf("\tMono camera\n");
    }

    ASI_CONTROL_CAPS control_caps;
    int ctrl_index = 0;
    ASIGetNumOfControls(camera_number, &ctrl_index);
    for (int i = 0; i < ctrl_index; i++) {
        ASIGetControlCaps(camera_number, i, &control_caps);
        printf(
             "\t%s (%ld <= %ld%s <= %ld): %s\n"
            ,control_caps.Name
            ,control_caps.MinValue
            ,control_caps.DefaultValue
            ,control_caps.IsWritable ? "*" : ""
            ,control_caps.MaxValue
            ,control_caps.Description
        );
    }

    long first_temp = 0;
    long current_temp = 0;
    long current_cool_percent = 0;
    ASI_BOOL is_auto = ASI_FALSE;
    if (ASIGetControlValue(camera_number, ASI_TEMPERATURE, &current_temp, &is_auto) == ASI_SUCCESS) {
        printf("sensor temperature: %02f\n", static_cast<float>(current_temp) / 10.0);
    } else {
        printf("Error getting sensor temperature\n");
        return -1;
    }

    /*
     * Establecemos como parámetros de captura 16bits, y usar todo el tamaño de sensor
     */
    ASI_IMG_TYPE image_type = ASI_IMG_RAW16;
    if (ASISetROIFormat(camera_number, max_width, max_height, 1, image_type) != ASI_SUCCESS) {
        printf("Error setting ROI format\n");
        return -1;
    }

    /*
     * Establecemos ganancia, exposición y ancho de banda USB a utilizar
     */
    long img_size = max_width * max_height * 2;
    unsigned char* img_buf = new unsigned char[img_size];
    ASISetControlValue(camera_number, ASI_GAIN, gain, ASI_FALSE);
    ASISetControlValue(camera_number, ASI_EXPOSURE, exposure * 1000, ASI_FALSE);
    ASISetControlValue(camera_number, ASI_BRIGHTNESS, offset, ASI_FALSE);
    ASISetControlValue(camera_number, ASI_BANDWIDTHOVERLOAD, 40, ASI_FALSE);
    size_t counter = 0;
    char counter_str[7];
    int old_target_temp = 30;
    ASI_EXPOSURE_STATUS status;
    bool temp_reached = set_temp >= 30;
    if (!temp_reached) {
        ASISetControlValue(camera_number, ASI_COOLER_ON, 1, ASI_FALSE);
        ASISetControlValue(camera_number, ASI_TARGET_TEMP, old_target_temp, ASI_FALSE);
    }
    while (counter < count) {
        /*
         * Iniciamos la exposición
         */
        printf("Exposing....\n");
        if (ASIStartExposure(camera_number, ASI_FALSE) != ASI_SUCCESS) {
            printf("Can not start exposure\n");
            return -1;
        }
        /*
         * Esperamos a que la exposición termine
         */
        usleep(10000);//10ms
        status = ASI_EXP_WORKING;
        while(status == ASI_EXP_WORKING)
        {
            ASIGetExpStatus(camera_number, &status);
            if (ASIGetControlValue(camera_number, ASI_TEMPERATURE, &current_temp, &is_auto) == ASI_SUCCESS
                    && ASIGetControlValue(camera_number, ASI_COOLER_POWER_PERC, &current_cool_percent, &is_auto) == ASI_SUCCESS) {
                printf("sensor temperature: %02.1fC <%ld>, cooler power: %ld %%, current target: %dC, set point: %d, temp reached: %s\n", static_cast<float>(current_temp) / 10.0, current_temp, current_cool_percent, old_target_temp, set_temp, temp_reached ? "true" : "false");
            }
            if (first_temp == 0 && current_temp != 0)  {
                first_temp = current_temp;
            }
            if (!temp_reached && first_temp != 0) {
                if (current_temp / 10 <= set_temp) {
                    cout << "TARGET TEMP REACHED, START SAVING EXPOSURES!!!!" << endl;
                    temp_reached = true;
                }
                if (!temp_reached) {
                    int target_temp = drive_temp(old_target_temp, current_temp / 10, set_temp);
                    cout << "target_temp " << target_temp << ", old_target_temp " << old_target_temp << endl;
                    if (target_temp != old_target_temp) {
                        cout << "SETTING CONSIGN TO " << target_temp << endl;
                        old_target_temp = target_temp;
                        ASISetControlValue(camera_number, ASI_TARGET_TEMP, target_temp, ASI_FALSE);
                    }
                }
            }
            usleep(250000);//250ms
        }
        if(status == ASI_EXP_SUCCESS)
        {
            /*
             * Si la exposición ha tenido éxito, creamos el fits
             */
            if (ASIGetDataAfterExp(camera_number, img_buf, img_size) != ASI_SUCCESS) {
                printf("Error getting capture data\n");
                return -1;
            }
            printf("Got exposure data.\n");
            if (temp_reached) {
                time_t now = time(nullptr);
                struct tm tm_now;
                if (gmtime_r(&now, &tm_now) == nullptr) {
                    printf("Cannot get gmt time!\n");
                    return -1;
                }
                snprintf(counter_str, 7, "%06lu", counter);
                counter_str[6] = 0;
                stringstream capture_stamp;
                capture_stamp << capture_name << "_" << (tm_now.tm_year + 1900)
                              << (tm_now.tm_mon < 9 ? "0" : "") << (tm_now.tm_mon + 1)
                              << (tm_now.tm_mday < 10 ? "0" : "") << tm_now.tm_mday
                              << "_" << (tm_now.tm_hour < 10 ? "0" : "") << tm_now.tm_hour
                              << (tm_now.tm_min < 10 ? "0" : "") << tm_now.tm_min
                              << (tm_now.tm_sec < 10 ? "0" : "") << tm_now.tm_sec
                              << "Z_" << counter_str << ".fit";
                cout << "File to save:" << capture_stamp.str() << endl;
                if (writeImage(capture_stamp.str().c_str(), max_width, max_height, img_buf, static_cast<double>(exposure) / 1000.0, static_cast<double>(current_temp) / 10.0, gain, offset) != 0) {
                    printf("Error writting data.\n");
                    return -1;
                }
                counter += 1;
            }
        } else {
            printf("Exposure failed: %d.\n", status);
            return -1;
        }

        /*
         * Paramos la exposición y liberamos recursos
         */
        if (ASIStopExposure(camera_number) != ASI_SUCCESS) {
            printf("Can not stop exposure\n");
        }
    }
    if (ASICloseCamera(camera_number) != ASI_SUCCESS) {
        printf("CloseCamera error,are you root?\n");
        return -1;
    }

    return 0;
}
