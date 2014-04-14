#include "getfacerequest.h"
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <detect-face/eHimage.h>

GetFaceRequest::GetFaceRequest(facemodel_t* faceModel, posemodel_t* poseModel, QObject *parent) :
    HttpRequestHandler(parent) {
    this->faceModel = faceModel;
    this->poseModel = poseModel;
}

GetFaceRequest::~GetFaceRequest() {
    if (faceModel) {
        delete faceModel;
    }
    if (poseModel) {
        delete poseModel;
    }
}

void GetFaceRequest::service(HttpRequest &request, HttpResponse &response) {
    QTemporaryFile* file = request.getUploadedFile("file");
    if (file) {
        bool error;
        QJsonDocument doc = getJSONFaces(file, error);
        if (error) {
            response.setStatus(500, "Internal processing error");
        }
        else {
            response.write(doc.toJson(QJsonDocument::Compact));
        }
    }
    else {
        response.setStatus(500, "Cannot find the file parameter");
    }
}

QJsonDocument GetFaceRequest::getJSONFaces(QFile *file, bool& error) {
    std::vector<bbox_t> faceBoxes;
    try {
        image_t* img = image_readJPG(file->fileName().toStdString().c_str());
        faceBoxes = facemodel_detect(faceModel, poseModel, img);
        image_delete(img);
    }
    catch (...) {
        error = true;
        return QJsonDocument();
    }

    QJsonArray faces;
    for (std::vector<bbox_t>::const_iterator bboxIter = faceBoxes.begin(); bboxIter != faceBoxes.end(); ++bboxIter) {
        bbox_t faceBox = (*bboxIter);
        QJsonObject faceParts;
        QJsonObject face;
        face["x1"] = faceBox.outer.x1;
        face["y1"] = faceBox.outer.y1;
        face["x2"] = faceBox.outer.x2;
        face["y2"] = faceBox.outer.y2;
        faceParts["face"] = face;
        QJsonArray parts;
        for (std::vector<fbox_t>::const_iterator fboxIter = faceBox.boxes.begin(); fboxIter != faceBox.boxes.end(); ++ fboxIter) {
            fbox_t box = (*fboxIter);
            QJsonObject part;
            part["x"] = (box.x1 + box.x2)/2.0;
            part["y"] = (box.y1 + box.y2)/2.0;
            parts.append(part);
        }
        faceParts["parts"] = parts;
        faceParts["pose"] = faceBox.pose;
        faces.append(faceParts);
    }
    QJsonDocument doc(faces);
    return doc;
}
