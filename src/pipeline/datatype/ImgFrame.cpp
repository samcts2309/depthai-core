#include "depthai/pipeline/datatype/ImgFrame.hpp"

#include "spdlog/fmt/fmt.h"

namespace dai {

ImgFrame::Serialized ImgFrame::serialize() const {
    return {data, raw};
}

ImgFrame::ImgFrame() : Buffer(std::make_shared<RawImgFrame>()), img(*dynamic_cast<RawImgFrame*>(raw.get())) {
    // set timestamp to now
    setTimestamp(std::chrono::steady_clock::now());
}

ImgFrame::ImgFrame(size_t size) : ImgFrame() {
    auto mem = std::make_shared<VectorMemory>();
    mem->resize(size);
    data = mem;
}

ImgFrame::ImgFrame(std::shared_ptr<RawImgFrame> ptr) : Buffer(std::move(ptr)), img(*dynamic_cast<RawImgFrame*>(raw.get())) {
    for(auto transformation : img.transformations) {
        switch(transformation.transformationType) {
            case RawImgTransformation::Transformation::Crop:
                transformations.emplace_back(std::make_shared<CropTransformation>(transformation));
                break;
            case RawImgTransformation::Transformation::Scale:
                transformations.emplace_back(std::make_shared<ScaleTransformation>(transformation));
                break;
            case RawImgTransformation::Transformation::Flip:
                break; // TODO
            case RawImgTransformation::Transformation::Pad:
                break; // TODO
            case RawImgTransformation::Transformation::Rotation:
                break; // TODO
            default:
                break;
        }
    }
}
// helpers

// getters
std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> ImgFrame::getTimestamp() const {
    using namespace std::chrono;
    return time_point<steady_clock, steady_clock::duration>{seconds(img.ts.sec) + nanoseconds(img.ts.nsec)};
}
std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> ImgFrame::getTimestampDevice() const {
    using namespace std::chrono;
    return time_point<steady_clock, steady_clock::duration>{seconds(img.tsDevice.sec) + nanoseconds(img.tsDevice.nsec)};
}
unsigned int ImgFrame::getInstanceNum() const {
    return img.instanceNum;
}
unsigned int ImgFrame::getCategory() const {
    return img.category;
}
int64_t ImgFrame::getSequenceNum() const {
    return img.sequenceNum;
}
unsigned int ImgFrame::getWidth() const {
    return img.fb.width;
}
unsigned int ImgFrame::getHeight() const {
    return img.fb.height;
}
RawImgFrame::Type ImgFrame::getType() const {
    return img.fb.type;
}
std::chrono::microseconds ImgFrame::getExposureTime() const {
    return std::chrono::microseconds(img.cam.exposureTimeUs);
}
int ImgFrame::getSensitivity() const {
    return img.cam.sensitivityIso;
}
int ImgFrame::getColorTemperature() const {
    return img.cam.wbColorTemp;
}
int ImgFrame::getLensPosition() const {
    return img.cam.lensPosition;
}

unsigned int ImgFrame::getSourceWidth() const {
    return img.sourceFb.width;
}
unsigned int ImgFrame::getSourceHeight() const {
    return img.sourceFb.height;
}

RawImgFrame ImgFrame::get() const {
    return img;
}

// setters
ImgFrame& ImgFrame::setTimestamp(std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> tp) {
    // Set timestamp from timepoint
    using namespace std::chrono;
    auto ts = tp.time_since_epoch();
    img.ts.sec = duration_cast<seconds>(ts).count();
    img.ts.nsec = duration_cast<nanoseconds>(ts).count() % 1000000000;
    return *this;
}
ImgFrame& ImgFrame::setTimestampDevice(std::chrono::time_point<std::chrono::steady_clock, std::chrono::steady_clock::duration> tp) {
    // Set timestamp from timepoint
    using namespace std::chrono;
    auto ts = tp.time_since_epoch();
    img.tsDevice.sec = duration_cast<seconds>(ts).count();
    img.tsDevice.nsec = duration_cast<nanoseconds>(ts).count() % 1000000000;
    return *this;
}
ImgFrame& ImgFrame::setInstanceNum(unsigned int instanceNum) {
    img.instanceNum = instanceNum;
    return *this;
}
ImgFrame& ImgFrame::setCategory(unsigned int category) {
    img.category = category;
    return *this;
}
ImgFrame& ImgFrame::setSequenceNum(int64_t sequenceNum) {
    img.sequenceNum = sequenceNum;
    return *this;
}
ImgFrame& ImgFrame::setWidth(unsigned int width) {
    img.fb.width = width;
    img.fb.stride = width;
    return *this;
}
ImgFrame& ImgFrame::setHeight(unsigned int height) {
    img.fb.height = height;
    return *this;
}
ImgFrame& ImgFrame::setSize(unsigned int width, unsigned int height) {
    setWidth(width);
    setHeight(height);
    return *this;
}
ImgFrame& ImgFrame::setSize(std::tuple<unsigned int, unsigned int> size) {
    setSize(std::get<0>(size), std::get<1>(size));
    return *this;
}
ImgFrame& ImgFrame::setSourceWidth(unsigned int width) {
    img.sourceFb.width = width;
    img.sourceFb.stride = width;
    return *this;
}
ImgFrame& ImgFrame::setSourceHeight(unsigned int height) {
    img.sourceFb.height = height;
    return *this;
}
ImgFrame& ImgFrame::setSourceSize(unsigned int width, unsigned int height) {
    setSourceWidth(width);
    setSourceHeight(height);
    return *this;
}
ImgFrame& ImgFrame::setSourceSize(std::tuple<unsigned int, unsigned int> size) {
    setSourceSize(std::get<0>(size), std::get<1>(size));
    return *this;
}
ImgFrame& ImgFrame::setType(RawImgFrame::Type type) {
    img.fb.type = type;
    img.fb.bytesPP = RawImgFrame::typeToBpp(img.fb.type);
    return *this;
}

void ImgFrame::copyTransformationsFrom(std::shared_ptr<dai::ImgFrame> sourceFrame) {
    transformations = sourceFrame->transformations;
    img.transformations = sourceFrame->get().transformations;
}

void ImgFrame::transSetPadding(float topPadding, float bottomPadding, float leftPadding, float rightPadding) {
    ;
}
void ImgFrame::transSetCrop(dai::Rect crop) {
    // Add a crop
    RawImgTransformation cropTransformation;
    auto cropNormalized = crop.normalize(getWidth(), getHeight());
    cropTransformation.crop = cropNormalized;
    cropTransformation.transformationType = RawImgTransformation::Transformation::Crop;

    // Add the transformation
    img.transformations.push_back(cropTransformation);
    transformations.emplace_back(std::make_shared<CropTransformation>(cropTransformation));

    // Set image size correctly
    auto cropDenormalized = crop.denormalize(getWidth(), getHeight());
    setWidth(cropDenormalized.width);
    setHeight(cropDenormalized.height);
}
void ImgFrame::transSetRotation(float rotationAngle, dai::Point2f rotationPoint) {}

void ImgFrame::transSetScale(float scaleFactorX, float scaleFactorY) {
    RawImgTransformation scaleTransformation;
    scaleTransformation.scaleFactorX = scaleFactorX;
    scaleTransformation.scaleFactorY = scaleFactorY;
    scaleTransformation.transformationType = RawImgTransformation::Transformation::Scale;

    // Add the transformation
    img.transformations.push_back(scaleTransformation);
    transformations.emplace_back(std::make_shared<ScaleTransformation>(scaleTransformation));

    // Correct the image sizes
    setWidth(getWidth() * scaleFactorX);
    setHeight(getHeight() * scaleFactorY);
}

dai::Point2f ScaleTransformation::trans(dai::Point2f point) {
    // Since we are dealing with relative coordinates, there is nothing to be done
    return point;
}

dai::Point2f ScaleTransformation::invTrans(dai::Point2f point) {
    // Since we are dealing with relative coordinates, there is nothing to be done
    return point;
}

dai::Point2f CropTransformation::trans(dai::Point2f point) {
    dai::Point2f returnPoint;
    float cropStartX = rawImgTransformation.crop.topLeft().x;
    float cropEndX = rawImgTransformation.crop.bottomRight().x;
    if(point.x < cropStartX) {
        returnPoint.x = 0;
    } else if(point.x > cropEndX) {
        returnPoint.x = 1;
    } else {
        returnPoint.x = (point.x - cropStartX) / (cropEndX - cropStartX);
    }

    float cropStartY = rawImgTransformation.crop.topLeft().y;
    float cropEndY = rawImgTransformation.crop.bottomRight().y;
    if(point.y < cropStartY) {
        returnPoint.y = 0;
    } else if(point.y > cropEndY) {
        returnPoint.y = 1;
    } else {
        returnPoint.y = (point.y - cropStartY) / (cropEndY - cropStartY);
    }
    return returnPoint;
}

dai::Point2f CropTransformation::invTrans(dai::Point2f point) {
    dai::Point2f returnPoint;
    float cropStartX = rawImgTransformation.crop.topLeft().x;
    float cropEndX = rawImgTransformation.crop.bottomRight().x;
    returnPoint.x = (point.x) * (cropEndX - cropStartX) + cropStartX;

    float cropStartY = rawImgTransformation.crop.topLeft().y;
    float cropEndY = rawImgTransformation.crop.bottomRight().y;
    returnPoint.y = (point.y) * (cropEndY - cropStartY) + cropStartY;

    return returnPoint;
}

}  // namespace dai
