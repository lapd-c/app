/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.4
 Written in Qt

                        graphicsedge.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2017 by Dimitris B. Kalamaras
    project site      : http://socnetv.org

 ***************************************************************************/

/*******************************************************************************
*     This program is free software: you can redistribute it and/or modify     *
*     it under the terms of the GNU General Public License as published by     *
*     the Free Software Foundation, either version 3 of the License, or        *
*     (at your option) any later version.                                      *
*                                                                              *
*     This program is distributed in the hope that it will be useful,          *
*     but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*     GNU General Public License for more details.                             *
*                                                                              *
*     You should have received a copy of the GNU General Public License        *
*     along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
********************************************************************************/

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QtDebug>		//used for qDebug messages
#include <cmath>

#include "graphicswidget.h"
#include "graphicsedge.h"
#include "graphicsnode.h"
#include "graphicsedgeweight.h"
#include "graphicsedgelabel.h"


static const double Pi = 3.14159265;
static double TwoPi = 2.0 * Pi;

static const int EDGE_DIRECTED = 0;
static const int EDGE_DIRECTED_OPPOSITE_EXISTS = 1;
static const int EDGE_RECIPROCAL_UNDIRECTED = 2;

GraphicsEdge::GraphicsEdge(GraphicsWidget *gw,
             GraphicsNode *from,
             GraphicsNode *to,
             const float &weight,
             const QString &label,
             const QString &color,
             const Qt::PenStyle &style,
             const int &type,
             const bool &drawArrows,
             const bool &bezier,
             const bool &weightNumbers, const bool &highlighting) : graphicsWidget(gw)
{

    graphicsWidget->scene()->addItem(this);  //add edge to scene to be displayed

    from->addOutLink( this );	//adds this to sourceNode
    to->addInLink( this );		//adds this to targetNode

    source=from;			//saves the sourceNode
    target=to;			//Saves the targetNode
    m_style = style;
    m_state = EDGE_STATE_REGULAR ;
    m_color=color;
    m_drawArrows=drawArrows;
    m_edgeType=type;

    m_directed_first = false;

    m_startOffset=source->size();  //used to offset edge from the centre of node
    m_endOffset=target->size();  //used to offset edge from the centre of node
    m_arrowSize=4;		//controls the width of the edge arrow
    eFrom = source->nodeNumber() ;
    eTo = target->nodeNumber() ;
    m_weight = weight ;
    m_Bezier = bezier;

    m_label = label;
    m_drawLabel = !m_label.isEmpty();
    m_drawWeightNumber = weightNumbers;

    qDebug()<< "GraphicsEdge::GraphicsEdge():  " << eFrom << "->" << eTo
            <<" = " << m_weight
            <<" label " << m_label
            <<" edgeType " << m_edgeType;

    if (m_drawWeightNumber) {
        addWeightNumber();
    }
    if (m_drawLabel)
        addLabel();

    m_hoverHighlighting = highlighting;

    setAcceptHoverEvents(true);
    setFlags(QGraphicsItem::ItemIsSelectable);

    //Edges have lower z than nodes. Nodes always appear above edges.
    setZValue(ZValueEdge);

    setBoundingRegionGranularity(0);
    //setCacheMode (QGraphicsItem::ItemCoordinateCache);

    m_path = new QPainterPath;
    m_path_shape = new QPainterPath;

    adjust();
}



void GraphicsEdge::showArrows(const bool &drawArrows){
    prepareGeometryChange();
    m_drawArrows=drawArrows;
    adjust();
}



void GraphicsEdge::removeRefs(){
    qDebug("GraphicsEdge: removeRefs()");
    source->deleteOutLink(this);
    target->deleteInLink(this);
}


void GraphicsEdge::setColor( const QString &str) {
    m_color=str;
    prepareGeometryChange();
}



QString GraphicsEdge::color() const{
    return m_color;
}

/**
 * @brief Called from Graph::graphSaveToPajekFormat()
 * @return
 */
QString GraphicsEdge::colorToPajek() {
    if (m_color.startsWith("#")) {
        return  ("RGB"+m_color.right( m_color.size()-1 )).toUpper()  ;
    }
    return m_color;
}

/**
 * @brief Called from MW when user wants to change an edge's weight.
    Updates both the width and the weightNumber
 * @param w
 */
void GraphicsEdge::setWeight(const float &w) {
    qDebug() << "GraphicsEdge::setWeight() " << w;
    prepareGeometryChange();
    m_weight = w;
    if (m_drawWeightNumber)
        weightNumber->setPlainText (QString::number(w));
}

float GraphicsEdge::weight() const {
    qDebug() << "GraphicsEdge::weight() " << m_weight;
    return m_weight;
}



void GraphicsEdge::addWeightNumber (){
    // create edge weight item
    double x = -20 + ( source->x() + target->x() ) / 2.0;
    double y = -20 + ( source->y() + target->y() ) / 2.0;
    weightNumber = new  GraphicsEdgeWeight (this, 7, QString::number(m_weight) );
    weightNumber-> setPos(x,y);
    weightNumber-> setDefaultTextColor (m_color);
    m_drawWeightNumber = true;
}

void GraphicsEdge::setWeightNumberVisibility (const bool &toggle) {
    if (m_drawWeightNumber) {
        if (toggle)
            weightNumber ->show();
        else
            weightNumber ->hide();
    }
    else{
        if (toggle)
            addWeightNumber();
    }

}




/**
 * @brief Called from MW when user wants to change an edge's label

 * @param label
 */
void GraphicsEdge::setLabel(const QString &label) {
    qDebug() << "GraphicsEdge::setLabel() " << label;
    prepareGeometryChange();
    m_label = label;
    if (m_drawLabel)
        edgeLabel->setPlainText (m_label);
}

QString GraphicsEdge::label() const {
    return m_label;
}


void GraphicsEdge::addLabel (){
    // create edge label item
    double x =  5+ ( source->x() + target->x() ) / 2.0;
    double y =  5+ ( source->y() + target->y() ) / 2.0;
    edgeLabel = new  GraphicsEdgeLabel (this, 7, m_label );
    edgeLabel-> setPos(x,y);
    edgeLabel-> setDefaultTextColor (m_color);
    m_drawLabel = true;
}

void GraphicsEdge::setLabelVisibility (const bool &toggle) {
    if (m_drawLabel) {
        if (toggle)
            edgeLabel ->show();
        else
            edgeLabel ->hide();
    }
    else{
        if (toggle)
            addLabel();
    }

}


void GraphicsEdge::setStartOffset(const int &offset){
    m_startOffset=offset;
}

void GraphicsEdge::setEndOffset(int offset){
    m_endOffset=offset;
}


GraphicsNode *GraphicsEdge::sourceNode() const {
    return source;
}

void GraphicsEdge::setSourceNode(GraphicsNode *node) {
    source = node;
    adjust();
}


GraphicsNode *GraphicsEdge::targetNode() const {
    return target;
}


void GraphicsEdge::setTargetNode(GraphicsNode *node){
    target = node;
    adjust();
}


int GraphicsEdge::sourceNodeNumber () {
    return eFrom;
}

int GraphicsEdge::targetNodeNumber() {
    return eTo;
}


/**
 * @brief Leaves some empty space (offset) from node -
 * make the edge weight appear on the centre of the edge
 */
void GraphicsEdge::adjust(){
   // qDebug() << "GraphicsEdge::adjust()";
    if (!source || !target)
        return;
    //QLineF line(mapFromItem(source, 0, 0), mapFromItem(target, 0, 0));
    QLineF line(source->x(), source->y(), target->x(), target->y());
    QPointF edgeOffset;
    line_length = line.length();
    line_dx = line.dx();
    line_dy = line.dy();
    if (source!=target) {
        edgeOffset = QPointF(
                    (line_dx * m_endOffset) / line_length,
                    (line_dy * m_endOffset) / line_length);
    }
    else edgeOffset = QPointF(0, 0);

    prepareGeometryChange();

    sourcePoint = line.p1() + edgeOffset ;
    targetPoint = line.p2() - edgeOffset ;

    if (m_edgeType == EDGE_DIRECTED_OPPOSITE_EXISTS ) {
        if (m_directed_first ) {
            sourcePoint -= QPointF(4,4);
            targetPoint -= QPointF(4,4);
        }
        else {
            sourcePoint += QPointF(4,4);
            targetPoint += QPointF(4,4);

        }

    }
    if (m_drawWeightNumber)
        weightNumber->setPos(
                    -20 + (source->x()+target->x())/2.0,
                    -20+ (source->y()+target->y())/2.0 );

    if (m_drawLabel)
        edgeLabel->setPos(
                    5+ (source->x()+target->x())/2.0,
                    5+ (source->y()+target->y())/2.0 );

    //Define the path upon which we' ll draw the line
    //QPainterPath line(sourcePoint);
    m_path =  new QPainterPath(sourcePoint);

    //Construct the path
    if (source!=target) {
        if ( !m_Bezier){
            //   qDebug()<< "*** GraphicsEdge::paint(). Constructing a line";
            m_path->lineTo(targetPoint);
        }
        else {
            qDebug() << "*** GraphicsEdge::paint(). Constructing a bezier curve";
        }
    }
    else { //self-link
        QPointF c1 = QPointF( targetPoint.x() -30,  targetPoint.y() -30 );
        QPointF c2 = QPointF( targetPoint.x() +30,  targetPoint.y() -30 );
//        qDebug()<< "*** GraphicsEdge::paint(). Constructing a bezier self curve c1 "
//                <<c1.x()<<","<<c1.y()  << " and c2 "<<c2.x()<<","<<c2.y();
        m_path->cubicTo( c1, c2, targetPoint);
    }

    //Draw the arrows only if we have different nodes
    //and the nodes are enough far apart from each other
    if (m_drawArrows && source!=target && line_length > 10) {
        angle = 0;
//        line_length = m_path->length();
//        line_dx = targetPoint.x()-sourcePoint.x();
//        line_dy = targetPoint.y()-sourcePoint.y();
        if ( line_length > 0 )
            angle = ::acos( line_dx / line_length );
        //		qDebug() << " acos() " << ::acos( line_dx  / line_length ) ;

        if ( line_dy  >= 0)
            angle = TwoPi - angle;


//            qDebug() << "*** GraphicsEdge::paint(). Constructing arrows. "
//                        "First Arrow at target node"
//                     << "target-source: " << line_dx
//                     << " length: " << line_length
//                     << " angle: "<< angle;

            QPointF destArrowP1 = targetPoint + QPointF(sin(angle - Pi / 3) * m_arrowSize,
                                                        cos(angle - Pi / 3) * m_arrowSize);
            QPointF destArrowP2 = targetPoint + QPointF(sin(angle - Pi + Pi / 3) * m_arrowSize,
                                                        cos(angle - Pi + Pi / 3) * m_arrowSize);
//            qDebug() << "*** GraphicsEdge::paint() destArrowP1 "
//                     <<  destArrowP1.x() << "," << destArrowP1.y()
//                      << "  destArrowP2 " <<  destArrowP2.x() << "," << destArrowP2.y();

            m_path->addPolygon ( QPolygonF()
                                 << targetPoint
                                 << destArrowP1
                                 << destArrowP2
                                 << targetPoint
                                 );

            if (m_edgeType == EDGE_RECIPROCAL_UNDIRECTED ) {
    //            qDebug() << "**** GraphicsEdge::paint() This edge is SYMMETRIC! "
    //                     << " So, we need to create Arrow at src node as well";
                QPointF srcArrowP1 = sourcePoint + QPointF(sin(angle +Pi / 3) * m_arrowSize,
                                                           cos(angle +Pi / 3) * m_arrowSize);
                QPointF srcArrowP2 = sourcePoint + QPointF(sin(angle +Pi - Pi  / 3) * m_arrowSize,
                                                           cos(angle +Pi - Pi / 3) * m_arrowSize);

                m_path->addPolygon ( QPolygonF()
                                     << sourcePoint
                                     << srcArrowP1
                                     << srcArrowP2
                                     <<sourcePoint
                                     );

            }
            else {
                // qDebug() << "*** GraphicsEdge::paint() Not symmetric edge. Finish";
            }


    }
    else {
//        qDebug()<< "*** GraphicsEdge::paint(). This edge is self-link - CONTINUE!";
    }


}



/**
 * @brief  Returns the shape of this edge as a QPainterPath in local coordinates.
 * The shape is used for many things, including collision detection, hit tests,
 * and for the QGraphicsScene::items() functions.
 * The default implementation calls boundingRect() to return a simple rectangular shape,
 * but we reimplement it to return a more accurate shape for non-rectangular items.
 * @return QPainterPath
 */
QPainterPath GraphicsEdge::shape () const {
    //qDebug()<<"GraphicsEdge::shape()";		//too many debug messages...
    *m_path_shape= QPainterPath(*m_path);
    m_path_shape->addPath(m_path->translated(1,1));
    m_path_shape->addPath(m_path->translated(-1,-1));
    return *m_path_shape;
} 


/**
 * @brief Defines the outer bounds of the edge as a rectangle;
 * All painting will be restricted to inside the edge's bounding rect.
 * Qt uses this bounding rect to determine whether the edge requires redrawing.
 * @return
 */
QRectF GraphicsEdge::boundingRect() const {
    if (!source || !target)
        return QRectF();
    return m_path->controlPointRect();
}


void GraphicsEdge::setDirectedWithOpposite(){
    qDebug()<< "GraphicsEdge::setDirectedWithOpposite()";
    prepareGeometryChange();
    m_edgeType = EDGE_DIRECTED_OPPOSITE_EXISTS;
    m_directed_first= true;
}




void GraphicsEdge::setUndirected(){
    qDebug()<< "GraphicsEdge::setUndirected()";
    prepareGeometryChange();
    m_edgeType = EDGE_RECIPROCAL_UNDIRECTED;
    m_directed_first= false;
    m_drawArrows = false;
    adjust();
}


bool GraphicsEdge::isUndirected() {
    return ( m_edgeType == EDGE_RECIPROCAL_UNDIRECTED ) ? true:false;
}


void GraphicsEdge::setStyle( const Qt::PenStyle  &style ) {
    m_style = style;
}

Qt::PenStyle GraphicsEdge::style() const{
    return m_style;

}

/**
 * @brief GraphicsEdge::pen
 * @return
 */
QPen GraphicsEdge::pen() const {

    switch (m_state) {
    case EDGE_STATE_REGULAR:
        //qDebug() << "GraphicsEdge::pen() - returning pen for state REGULAR"  ;
        if (m_weight < 0 ){
            return  QPen(QColor(m_color), width(), Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
        }
        return QPen(QColor(m_color), width(), style(), Qt::RoundCap, Qt::RoundJoin);
        break;
    case EDGE_STATE_HIGHLIGHT: // selected
        //qDebug() << "GraphicsEdge::pen() - returning pen for state HIGHLIGHTED"  ;
        return QPen(QColor("red"), width(), style(), Qt::RoundCap, Qt::RoundJoin);
    case EDGE_STATE_HOVER: // hover
        //qDebug() << "GraphicsEdge::pen() - returning pen for state HOVER"  ;
        return QPen(QColor("red"), width()+1, style(), Qt::RoundCap, Qt::RoundJoin);
    default:
        //qDebug() << "GraphicsEdge::pen() - returning pen for state DEFAULT"  ;
        return QPen(QColor(m_color), width(), style(), Qt::RoundCap, Qt::RoundJoin);
    }

}

void GraphicsEdge::setState(const int &state) {
    //NOTE: DO NOT USE HERE: prepareGeometryChange()
    m_state=state;
}

void GraphicsEdge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *){
    if (!source || !target)
        return;

    //qDebug() <<"@@@ GraphicsEdge::paint()";

     //if the edge is being dragged around, darken it!
     if (option->state & QStyle::State_Selected) {
         setZValue(ZValueEdgeHighlighted);
         setState(EDGE_STATE_HOVER);
     }
     else if (option->state & QStyle::State_MouseOver) {
         if (m_hoverHighlighting) {
             setZValue(ZValueEdgeHighlighted);
             setState(EDGE_STATE_HOVER);
         }
     }
     else if (m_state==EDGE_STATE_HIGHLIGHT){
         if (m_hoverHighlighting) {
             setZValue(ZValueEdgeHighlighted);
             setState(EDGE_STATE_HIGHLIGHT);
         }
     }
     else {
         setZValue(ZValueEdge);
         setState(EDGE_STATE_REGULAR);
     }
    // set painter pen to correct edge pen
    painter->setPen(pen());

    painter->drawPath(*m_path);
}


/** 
    Controls the width of the edge; is a function of edge weight
*/
float GraphicsEdge::width() const{
    if ( fabs(m_weight) > 1  )  {
        return 1+log(fabs(m_weight)) ;
    }
    return fabs(m_weight) ;
}



/**
 * @brief Called from GraphicsNode to change the edge state and highlight it.
 * This is done, when the user hovers over the node.
 * @param flag
 */
void GraphicsEdge::highlight(const bool &flag) {
    //qDebug()<< "GraphicsEdge::highlight() - " << flag;
    if (flag && m_hoverHighlighting) {
        prepareGeometryChange();
        setState(EDGE_STATE_HIGHLIGHT);
    }
    else {
        prepareGeometryChange();
        setState(EDGE_STATE_REGULAR);
    }
}

/**
 * @brief Toggles edge highlighting
 * @param toggle
 */
void GraphicsEdge::setHighlighting(const bool &toggle) {
    m_hoverHighlighting = toggle;

}

/** handles the events of a click on an edge*/
void GraphicsEdge::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    qDebug() <<"GraphicsEdge::mousePressEvent()";
    QGraphicsItem::mousePressEvent(event);
}




GraphicsEdge::~GraphicsEdge(){
    qDebug() << "GraphicsEdge::~GraphicsEdge() - self-destructing edge " << sourceNodeNumber()<< "->" << targetNodeNumber();
    removeRefs();
    if (m_drawWeightNumber)
        graphicsWidget->removeItem(weightNumber);
    if (m_drawLabel)
        graphicsWidget->removeItem(edgeLabel);

    this->hide();

    graphicsWidget->removeItem(this);

    if ( ! m_path->isEmpty())
         delete m_path;

}

