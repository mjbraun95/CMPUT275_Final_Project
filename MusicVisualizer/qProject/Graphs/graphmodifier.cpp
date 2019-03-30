#include "graphmodifier.h"
#include <QtDataVisualization/qcategory3daxis.h>
#include <QtDataVisualization/qvalue3daxis.h>
#include <QtDataVisualization/qbardataproxy.h>
#include <QtDataVisualization/q3dscene.h>
#include <QtDataVisualization/q3dcamera.h>
#include <QtDataVisualization/qbar3dseries.h>
#include <QtDataVisualization/q3dtheme.h>
#include <QtCore/QTime>
#include <QtWidgets/QComboBox>
#include <QtCore/qmath.h>
#include <vector>
#include <FFTStuff/audio_file.h>

using namespace QtDataVisualization;

vector<vector<double>> calcData()
{
    audio_file input_file("440.mp3", 44100);
    double* data;
    unsigned long size;
    input_file.decode(&data, &size);

    double N = 2048;
    double sample_rate = 44100;

    vector<double> appearance;
    int bins = 100 / (sample_rate / N);

    double index = 0;
    // to access the frequency bins at time t
    // use f_bins_collection[t]
    // to access a certain frequency bin (index i) at time t
    // use f_bins_collection[t][i]

    vector<vector<double>> f_bins_collection = input_file.f_domain(&data, &size);

//    for (auto iter : f_bins_collection) {

        // if (iter[bins] > 100) {
//            appearance.push_back(index * (N / sample_rate));
            // cout << "iter[" << bins << "]: " << iter[bins] << "; freq: " << (index * (N / sample_rate)) << endl;
        // }

//        index += 1;
//    }
    return f_bins_collection;
}

const QString celsiusString = QString(QChar(0xB0)) + "C";

GraphModifier::GraphModifier(Q3DBars *bargraph)
    : m_graph(bargraph),
      m_xRotation(0.0f),
      m_yRotation(0.0f),
      m_fontSize(30),
      m_segments(4),
      m_subSegments(3),
      m_minval(-20.0f),
      m_maxval(20.0f),
      m_magnitudeAxis(new QValue3DAxis),
      m_timeAxis(new QCategory3DAxis),
      m_freqAxis(new QCategory3DAxis),
      m_primarySeries(new QBar3DSeries),
      m_secondarySeries(new QBar3DSeries),
      m_barMesh(QAbstract3DSeries::MeshBevelBar),
      m_smooth(false)
{
    m_graph->setShadowQuality(QAbstract3DGraph::ShadowQualitySoftMedium);
    m_graph->activeTheme()->setBackgroundEnabled(false);
    m_graph->activeTheme()->setFont(QFont("Times New Roman", m_fontSize));
    m_graph->activeTheme()->setLabelBackgroundEnabled(true);
    m_graph->setMultiSeriesUniform(true);


    m_freqBuckets << "January";
    m_freqBuckets << "February" << "March" << "April" << "May" << "June" << "July" << "August" << "September" << "October" << "November" << "December";
    m_timeBuckets << "2006" << "2007" << "2008" << "2009" << "2010" << "2011" << "2012" << "2013";

    m_magnitudeAxis->setTitle("Magnitude");
    m_magnitudeAxis->setSegmentCount(m_segments);
    m_magnitudeAxis->setSubSegmentCount(m_subSegments);
    m_magnitudeAxis->setRange(m_minval, m_maxval);
    m_magnitudeAxis->setLabelFormat(QString(QStringLiteral("%.1f ") + celsiusString));
    m_magnitudeAxis->setLabelAutoRotation(30.0f);
    m_magnitudeAxis->setTitleVisible(true);

    m_timeAxis->setTitle("Time");
    m_timeAxis->setLabelAutoRotation(30.0f);
    m_timeAxis->setTitleVisible(true);
    m_freqAxis->setTitle("Frequency");
    m_freqAxis->setLabelAutoRotation(30.0f);
    m_freqAxis->setTitleVisible(true);

    m_graph->setValueAxis(m_magnitudeAxis);
    m_graph->setRowAxis(m_timeAxis);
    m_graph->setColumnAxis(m_freqAxis);

    m_primarySeries->setItemLabelFormat(QStringLiteral("Oulu - @colLabel @rowLabel: @valueLabel"));
    m_primarySeries->setMesh(QAbstract3DSeries::MeshBevelBar);
    m_primarySeries->setMeshSmooth(false);

    m_secondarySeries->setItemLabelFormat(QStringLiteral("Helsinki - @colLabel @rowLabel: @valueLabel"));
    m_secondarySeries->setMesh(QAbstract3DSeries::MeshBevelBar);
    m_secondarySeries->setMeshSmooth(false);
    m_secondarySeries->setVisible(false);

    m_graph->addSeries(m_primarySeries);
    m_graph->addSeries(m_secondarySeries);

    changePresetCamera();

    resetTemperatureData();

    // Set up property animations for zooming to the selected bar
    Q3DCamera *camera = m_graph->scene()->activeCamera();
    m_defaultAngleX = camera->xRotation();
    m_defaultAngleY = camera->yRotation();
    m_defaultZoom = camera->zoomLevel();
    m_defaultTarget = camera->target();

    m_animationCameraX.setTargetObject(camera);
    m_animationCameraY.setTargetObject(camera);
    m_animationCameraZoom.setTargetObject(camera);
    m_animationCameraTarget.setTargetObject(camera);

    m_animationCameraX.setPropertyName("xRotation");
    m_animationCameraY.setPropertyName("yRotation");
    m_animationCameraZoom.setPropertyName("zoomLevel");
    m_animationCameraTarget.setPropertyName("target");

    int duration = 1700;
    m_animationCameraX.setDuration(duration);
    m_animationCameraY.setDuration(duration);
    m_animationCameraZoom.setDuration(duration);
    m_animationCameraTarget.setDuration(duration);

    // The zoom always first zooms out above the graph and then zooms in
    qreal zoomOutFraction = 0.3;
    m_animationCameraX.setKeyValueAt(zoomOutFraction, QVariant::fromValue(0.0f));
    m_animationCameraY.setKeyValueAt(zoomOutFraction, QVariant::fromValue(90.0f));
    m_animationCameraZoom.setKeyValueAt(zoomOutFraction, QVariant::fromValue(50.0f));
    m_animationCameraTarget.setKeyValueAt(zoomOutFraction,
                                          QVariant::fromValue(QVector3D(0.0f, 0.0f, 0.0f)));
}

GraphModifier::~GraphModifier()
{
    delete m_graph;
}

void GraphModifier::resetTemperatureData()
{
    // Set up data
    vector<vector<double>> data1 = calcData();
    std::cout << "Resetting Data" << std::endl;
    std::cout << data1[1][1] << std::endl;
    static const float tempOulu[8][12] = {
        {-6.7f, -11.7f, -9.7f, 3.3f, 9.2f, 14.0f, 16.3f, 17.8f, 10.2f, 2.1f, -2.6f, -0.3f},    // 2006
        {-6.8f, -13.3f, 0.2f, 1.5f, 7.9f, 13.4f, 16.1f, 15.5f, 8.2f, 5.4f, -2.6f, -0.8f},      // 2007
        {-4.2f, -4.0f, -4.6f, 1.9f, 7.3f, 12.5f, 15.0f, 12.8f, 7.6f, 5.1f, -0.9f, -1.3f},      // 2008
        {-7.8f, -8.8f, -4.2f, 0.7f, 9.3f, 13.2f, 15.8f, 15.5f, 11.2f, 0.6f, 0.7f, -8.4f},      // 2009
        {-14.4f, -12.1f, -7.0f, 2.3f, 11.0f, 12.6f, 18.8f, 13.8f, 9.4f, 3.9f, -5.6f, -13.0f},  // 2010
        {-9.0f, -15.2f, -3.8f, 2.6f, 8.3f, 15.9f, 18.6f, 14.9f, 11.1f, 5.3f, 1.8f, -0.2f},     // 2011
        {-8.7f, -11.3f, -2.3f, 0.4f, 7.5f, 12.2f, 16.4f, 14.1f, 9.2f, 3.1f, 0.3f, -12.1f},     // 2012
        {-7.9f, -5.3f, -9.1f, 0.8f, 11.6f, 16.6f, 15.9f, 15.5f, 11.2f, 4.0f, 0.1f, -1.9f}      // 2013
    };

    static const float tempHelsinki[8][12] = {
        {-3.7f, -7.8f, -5.4f, 3.4f, 10.7f, 15.4f, 18.6f, 18.7f, 14.3f, 8.5f, 2.9f, 4.1f},      // 2006
        {-1.2f, -7.5f, 3.1f, 5.5f, 10.3f, 15.9f, 17.4f, 17.9f, 11.2f, 7.3f, 1.1f, 0.5f},       // 2007
        {-0.6f, 1.2f, 0.2f, 6.3f, 10.2f, 13.8f, 18.1f, 15.1f, 10.1f, 9.4f, 2.5f, 0.4f},        // 2008
        {-2.9f, -3.5f, -0.9f, 4.7f, 10.9f, 14.0f, 17.4f, 16.8f, 13.2f, 4.1f, 2.6f, -2.3f},     // 2009
        {-10.2f, -8.0f, -1.9f, 6.6f, 11.3f, 14.5f, 21.0f, 18.8f, 12.6f, 6.1f, -0.5f, -7.3f},   // 2010
        {-4.4f, -9.1f, -2.0f, 5.5f, 9.9f, 15.6f, 20.8f, 17.8f, 13.4f, 8.9f, 3.6f, 1.5f},       // 2011
        {-3.5f, -3.2f, -0.7f, 4.0f, 11.1f, 13.4f, 17.3f, 15.8f, 13.1f, 6.4f, 4.1f, -5.1f},     // 2012
        {-4.8f, -1.8f, -5.0f, 2.9f, 12.8f, 17.2f, 18.0f, 17.1f, 12.5f, 7.5f, 4.5f, 2.3f}       // 2013
    };

    // Create data arrays
    QBarDataArray *dataSet = new QBarDataArray;
    QBarDataArray *dataSet2 = new QBarDataArray;
    QBarDataRow *dataRow;
    QBarDataRow *dataRow2;

    dataSet->reserve(m_timeBuckets.size());
    for (int year = 0; year < m_timeBuckets.size(); year++) {
        // Create a data row
        dataRow = new QBarDataRow(m_freqBuckets.size());
        dataRow2 = new QBarDataRow(m_freqBuckets.size());
        for (int month = 0; month < m_freqBuckets.size(); month++) {
            // Add data to the row
            (*dataRow)[month].setValue(tempOulu[year][month]);
            (*dataRow2)[month].setValue(tempHelsinki[year][month]);
        }
        // Add the row to the set
        dataSet->append(dataRow);
        dataSet2->append(dataRow2);
    }

    // Add data to the data proxy (the data proxy assumes ownership of it)
    m_primarySeries->dataProxy()->resetArray(dataSet, m_timeBuckets, m_freqBuckets);
    m_secondarySeries->dataProxy()->resetArray(dataSet2, m_timeBuckets, m_freqBuckets);
}

void GraphModifier::changeRange(int range)
{
    if (range >= m_timeBuckets.count())
        m_timeAxis->setRange(0, m_timeBuckets.count() - 1);
    else
        m_timeAxis->setRange(range, range);
}

void GraphModifier::changeStyle(int style)
{
    QComboBox *comboBox = qobject_cast<QComboBox *>(sender());
    if (comboBox) {
        m_barMesh = QAbstract3DSeries::Mesh(comboBox->itemData(style).toInt());
        m_primarySeries->setMesh(m_barMesh);
        m_secondarySeries->setMesh(m_barMesh);
    }
}

void GraphModifier::changePresetCamera()
{
    m_animationCameraX.stop();
    m_animationCameraY.stop();
    m_animationCameraZoom.stop();
    m_animationCameraTarget.stop();

    // Restore camera target in case animation has changed it
    m_graph->scene()->activeCamera()->setTarget(QVector3D(0.0f, 0.0f, 0.0f));

    static int preset = Q3DCamera::CameraPresetFront;

    m_graph->scene()->activeCamera()->setCameraPreset((Q3DCamera::CameraPreset)preset);

    if (++preset > Q3DCamera::CameraPresetDirectlyBelow)
        preset = Q3DCamera::CameraPresetFrontLow;
}

void GraphModifier::changeTheme(int theme)
{
    Q3DTheme *currentTheme = m_graph->activeTheme();
    currentTheme->setType(Q3DTheme::Theme(theme));
    emit backgroundEnabledChanged(currentTheme->isBackgroundEnabled());
    emit gridEnabledChanged(currentTheme->isGridEnabled());
    emit fontChanged(currentTheme->font());
    emit fontSizeChanged(currentTheme->font().pointSize());
}

void GraphModifier::changeLabelBackground()
{
    m_graph->activeTheme()->setLabelBackgroundEnabled(!m_graph->activeTheme()->isLabelBackgroundEnabled());
}

void GraphModifier::changeSelectionMode(int selectionMode)
{
    QComboBox *comboBox = qobject_cast<QComboBox *>(sender());
    if (comboBox) {
        int flags = comboBox->itemData(selectionMode).toInt();
        m_graph->setSelectionMode(QAbstract3DGraph::SelectionFlags(flags));
    }
}

void GraphModifier::changeFont(const QFont &font)
{
    QFont newFont = font;
    m_graph->activeTheme()->setFont(newFont);
}

void GraphModifier::changeFontSize(int fontsize)
{
    m_fontSize = fontsize;
    QFont font = m_graph->activeTheme()->font();
    font.setPointSize(m_fontSize);
    m_graph->activeTheme()->setFont(font);
}

void GraphModifier::shadowQualityUpdatedByVisual(QAbstract3DGraph::ShadowQuality sq)
{
    int quality = int(sq);
    // Updates the UI component to show correct shadow quality
    emit shadowQualityChanged(quality);
}

void GraphModifier::changeLabelRotation(int rotation)
{
    m_magnitudeAxis->setLabelAutoRotation(float(rotation));
    m_freqAxis->setLabelAutoRotation(float(rotation));
    m_timeAxis->setLabelAutoRotation(float(rotation));
}

void GraphModifier::setAxisTitleVisibility(bool enabled)
{
    m_magnitudeAxis->setTitleVisible(enabled);
    m_freqAxis->setTitleVisible(enabled);
    m_timeAxis->setTitleVisible(enabled);
}

void GraphModifier::setAxisTitleFixed(bool enabled)
{
    m_magnitudeAxis->setTitleFixed(enabled);
    m_freqAxis->setTitleFixed(enabled);
    m_timeAxis->setTitleFixed(enabled);
}

void GraphModifier::zoomToSelectedBar()
{
    m_animationCameraX.stop();
    m_animationCameraY.stop();
    m_animationCameraZoom.stop();
    m_animationCameraTarget.stop();

    Q3DCamera *camera = m_graph->scene()->activeCamera();
    float currentX = camera->xRotation();
    float currentY = camera->yRotation();
    float currentZoom = camera->zoomLevel();
    QVector3D currentTarget = camera->target();

    m_animationCameraX.setStartValue(QVariant::fromValue(currentX));
    m_animationCameraY.setStartValue(QVariant::fromValue(currentY));
    m_animationCameraZoom.setStartValue(QVariant::fromValue(currentZoom));
    m_animationCameraTarget.setStartValue(QVariant::fromValue(currentTarget));

    QPoint selectedBar = m_graph->selectedSeries()
            ? m_graph->selectedSeries()->selectedBar()
            : QBar3DSeries::invalidSelectionPosition();

    if (selectedBar != QBar3DSeries::invalidSelectionPosition()) {
        // Normalize selected bar position within axis range to determine target coordinates
        QVector3D endTarget;
        float xMin = m_graph->columnAxis()->min();
        float xRange = m_graph->columnAxis()->max() - xMin;
        float zMin = m_graph->rowAxis()->min();
        float zRange = m_graph->rowAxis()->max() - zMin;
        endTarget.setX((selectedBar.y() - xMin) / xRange * 2.0f - 1.0f);
        endTarget.setZ((selectedBar.x() - zMin) / zRange * 2.0f - 1.0f);

        // Rotate the camera so that it always points approximately to the graph center
        qreal endAngleX = 90.0 - qRadiansToDegrees(qAtan(qreal(endTarget.z() / endTarget.x())));
        if (endTarget.x() > 0.0f)
            endAngleX -= 180.0f;
        float barValue = m_graph->selectedSeries()->dataProxy()->itemAt(selectedBar.x(),
                                                                        selectedBar.y())->value();
        float endAngleY = barValue >= 0.0f ? 30.0f : -30.0f;
        if (m_graph->valueAxis()->reversed())
            endAngleY *= -1.0f;

        m_animationCameraX.setEndValue(QVariant::fromValue(float(endAngleX)));
        m_animationCameraY.setEndValue(QVariant::fromValue(endAngleY));
        m_animationCameraZoom.setEndValue(QVariant::fromValue(250));
        m_animationCameraTarget.setEndValue(QVariant::fromValue(endTarget));
    } else {
        // No selected bar, so return to the default view
        m_animationCameraX.setEndValue(QVariant::fromValue(m_defaultAngleX));
        m_animationCameraY.setEndValue(QVariant::fromValue(m_defaultAngleY));
        m_animationCameraZoom.setEndValue(QVariant::fromValue(m_defaultZoom));
        m_animationCameraTarget.setEndValue(QVariant::fromValue(m_defaultTarget));
    }

    m_animationCameraX.start();
    m_animationCameraY.start();
    m_animationCameraZoom.start();
    m_animationCameraTarget.start();
}

void GraphModifier::changeShadowQuality(int quality)
{
    QAbstract3DGraph::ShadowQuality sq = QAbstract3DGraph::ShadowQuality(quality);
    m_graph->setShadowQuality(sq);
    emit shadowQualityChanged(quality);
}

void GraphModifier::rotateX(int rotation)
{
    m_xRotation = rotation;
    m_graph->scene()->activeCamera()->setCameraPosition(m_xRotation, m_yRotation);
}

void GraphModifier::rotateY(int rotation)
{
    m_yRotation = rotation;
    m_graph->scene()->activeCamera()->setCameraPosition(m_xRotation, m_yRotation);
}

void GraphModifier::setBackgroundEnabled(int enabled)
{
    m_graph->activeTheme()->setBackgroundEnabled(bool(enabled));
}

void GraphModifier::setGridEnabled(int enabled)
{
    m_graph->activeTheme()->setGridEnabled(bool(enabled));
}

void GraphModifier::setSmoothBars(int smooth)
{
    m_smooth = bool(smooth);
    m_primarySeries->setMeshSmooth(m_smooth);
    m_secondarySeries->setMeshSmooth(m_smooth);
}

void GraphModifier::setSeriesVisibility(int enabled)
{
    m_secondarySeries->setVisible(bool(enabled));
}

void GraphModifier::setReverseValueAxis(int enabled)
{
    m_graph->valueAxis()->setReversed(enabled);
}

void GraphModifier::setReflection(bool enabled)
{
    m_graph->setReflection(enabled);
}