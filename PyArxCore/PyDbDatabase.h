#pragma once
#include "stdafx.h"
#include "dbmain.h"
#include "PyRxObject.h"
#include "PyRxClass.h"

// Forward declarations only for wrapper types referenced by this class's own
// signatures. SDK raw types (AcDbDate, AcDbIdMapping, AcDbAnnotationScale,
// AcDbTransactionManager, AcDbObjectContextManager, AcDbLayerStateManager,
// AcDbDatabaseReactor, AcDbObjectIdGraph) need no forward decl here; their
// Python wrappers live in their own PyDb*.h files and are bound separately.
class PyDbObject;



class PyDbDatabase : public PyRxObject {
	PyWrapper_H(DbDatabase)
public:
	PyDbDatabase(AcDbDatabase* ptr = nullptr) : PyRxObject(ptr, false, false) {}
	PyDbDatabase(AcDbDatabase* ptr, bool autodelete) : PyRxObject(ptr, autodelete, false) {}
	PyDbDatabase(bool buildDefaultDrawing, bool noDocument) :
		PyDbDatabase(new AcDbDatabase(buildDefaultDrawing, noDocument), true) {
	}

	bool isBeingDestroyed() const;
	AcDbObjectId addAcDbObject(PyDbObject& pObj);
	Acad::ErrorStatus markObjectNonPersistent(AcDbObjectId id, bool value);
	bool isObjectNonPersistent(AcDbObjectId id);

	// --- 便捷方法：添加入库 ---
	uint64_t addToModelspace(PyDbObject& entity);
	uint64_t addToCurrentspace(PyDbObject& entity);
	uint64_t addToBlock(uint64_t blockId, PyDbObject& entity);

	// --- 遍历对象 ---
	py::list objectIds(const std::string& className = "AcDbEntity") const;

	// --- 系统变量统一访问 ---
	py::object getSysVar(const std::string& name) const;
	bool setSysVar(const std::string& name, const py::object& value);

	// --- 文件读写 ---
	bool dxfIn(const std::string& filename);
	bool dxfOut(const std::string& filename, int precision = 16);

	template <typename SymbolTableType>
	SymbolTableType* getSymbolTable(OpenMode mode = OpenMode::kForWrite) {
		SymbolTableType* pTable = nullptr;
		Acad::ErrorStatus es = impObj()->getSymbolTable(pTable, mode);
		if (es != Acad::eOk) {
			// 护栏：写打开失败（库只读 / 记录只读 / xref）抛清晰 Python 异常，
			// 不再让 ARX 内部弹 !dbobji.cpp eNotOpenForWrite 对话框。
			AcString esText = acadErrorStatusText(es);
			const char* esUtf8 = esText.utf8Ptr();
			throw std::runtime_error(std::string("getSymbolTable(kForWrite) 失败: ") +
				(esUtf8 ? esUtf8 : "") +
				"；需以可写方式打开数据库或另存副本（纯查询请显式传 kForRead=0）");
		}
		return pTable;
	}
	AcDbBlockTable* getBlockTable(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbLayerTable* getLayerTable(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbTextStyleTable* getTextStyleTable(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbLinetypeTable* getLinetypeTable(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbViewTable* getViewTable(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbUCSTable* getUCSTable(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbViewportTable* getViewportTable(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbRegAppTable* getRegAppTable(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbDimStyleTable* getDimStyleTable(AcDb::OpenMode mode = AcDb::kForWrite);

	// C++ 式手工打开对象（等价 acdbOpenObject）：不需要 PyDbTrans 也能拿对象，
	// 用完必须自己 obj.close()。mode: 0=读 1=写 2=通知（0/1 对应 kForRead/kForWrite）
	// 默认写模式：图元/表/符号表的增改默认以 kForWrite 打开，纯查询请显式传 0。
	PyDbObject openObject(uint64_t objectId, int mode = 1);

	AcDbDictionary* getGroupDictionary(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbDictionary* getMLStyleDictionary(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbDictionary* getLayoutDictionary(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbDictionaryWithDefault* getPlotStyleNameDictionary(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbDictionary* getMaterialDictionary(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbDictionary* getVisualStyleDictionary(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbDictionary* getColorDictionary(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbDictionary* getScaleListDictionary(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbDictionary* getNamedObjectsDictionary(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbDictionary* getPlotSettingsDictionary(AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbDictionary* getDataLinkDictionary(AcDb::OpenMode mode = AcDb::kForWrite);
	// 数据库对象管理器相关 
	AcDbObjectId getSectionManager() const;

	AcDbSectionManager* getSectionManager(AcDb::OpenMode mode) const;
	AcDbDataLinkManager* getDataLinkManager() const;
	Acad::ErrorStatus updateDataLink(AcDb::UpdateDirection nDir, AcDb::UpdateOption nOption);
	// 数据链接ID获取
	AcDbObjectId dataLinkDictionaryId() const;
	// 符号表ID获取 
	AcDbObjectId blockTableId() const;
	AcDbObjectId layerTableId() const;
	AcDbObjectId textStyleTableId() const;
	AcDbObjectId linetypeTableId() const;
	AcDbObjectId viewTableId() const;
	AcDbObjectId UCSTableId() const;
	AcDbObjectId viewportTableId() const;
	AcDbObjectId regAppTableId() const;
	AcDbObjectId dimStyleTableId() const;

	// 字典ID获取 
	AcDbObjectId mLStyleDictionaryId() const;
	AcDbObjectId groupDictionaryId() const;
	AcDbObjectId layoutDictionaryId() const;
	AcDbObjectId plotStyleNameDictionaryId() const;
	AcDbObjectId materialDictionaryId() const;
	AcDbObjectId visualStyleDictionaryId() const;
	AcDbObjectId colorDictionaryId() const;
	AcDbObjectId scaleListDictionaryId() const;
	AcDbObjectId namedObjectsDictionaryId() const;
	AcDbObjectId plotSettingsDictionaryId() const;

	// 特殊对象ID
	AcDbObjectId layerZero() const;
	AcDbObjectId continuousLinetype() const;
	AcDbObjectId byLayerLinetype() const;
	AcDbObjectId byBlockLinetype() const;
	AcDbObjectId globalMaterial() const;
	AcDbObjectId byLayerMaterial() const;
	AcDbObjectId byBlockMaterial() const;
	AcDbObjectId paperSpaceVportId() const;
	AcDbObjectId currentSpaceId() const;
	Acad::ErrorStatus reclaimMemoryFromErasedObjects(const PyDbObjectIdArray& erasedObjects);
	PyDbObjectIdArray purge(const PyDbObjectIdArray& ids);
	AcDbObjectId getPyDbObjectId(bool createIfNotFound, const AcDbHandle& objHandle, Adesk::UInt32 xRefId);
	AcString classDxfName(const PyRxClass& pClass);
	bool hasClass(const PyRxClass& pClass) const;
	Acad::ErrorStatus readDwgFile(AcString fileName, AcDbDatabase::OpenMode shmode, bool bAllowCPConversion, const AcString& wszPassword);
	Acad::ErrorStatus readDwgFile(AcDwgFileHandle* pDwgFile, bool bAllowCPConversion, const AcString& wszPassword);
	bool needsRecovery() const;
	void setFullSaveRequired();
	// 文件保存相关 
	Acad::ErrorStatus saveAs(AcString fileName, const SecurityParams* pSecParams = nullptr);
	Acad::ErrorStatus saveAs(AcString fileName, bool bBakAndRename, AcDb::AcDbDwgVersion dwgVer, const SecurityParams* pSecParams = nullptr);
	Acad::ErrorStatus saveAs(AcDwgFileHandle* pOutputFiler, const SecurityParams* pSecParams = nullptr, bool crashing = false);
	Acad::ErrorStatus save();

	// DXF输入输出
	Acad::ErrorStatus dxfIn(AcString dxfFilename, AcString logFilename);
	Acad::ErrorStatus dxfIn(IAcReadStream* pInputStream, AcString logFilename);
	Acad::ErrorStatus dxfOut(AcString fileName, int precision = 16, AcDb::AcDbDwgVersion dwgVer = AcDb::kDHL_CURRENT, bool saveThumbnailImage = false);
	Acad::ErrorStatus dxfOutEx(AcString fileName, int precision, AcDb::AcDbDwgVersion dwgVer, int nFlags, int nCodePage);

	// 文件操作 
	Acad::ErrorStatus closeInput(bool bCloseFile = true);

	// 数据库统计信息
	int approxNumObjects() const;
	Adesk::Int32 numberOfSaves() const;

	// 文件信息 
	AcString originalFileName() const;

	// 版本信息（DWG格式）
	AcDb::AcDbDwgVersion lastSavedAsVersion() const;
	AcDb::AcDbDwgVersion originalFileVersion() const;
	AcDb::AcDbDwgVersion originalFileSavedByVersion() const;

	// 版本信息（维护版本） 
	AcDb::MaintenanceReleaseVersion lastSavedAsMaintenanceVersion() const;
	AcDb::MaintenanceReleaseVersion originalFileMaintenanceVersion() const;
	AcDb::MaintenanceReleaseVersion originalFileSavedByMaintenanceVersion() const;
	AcDwgFileHandle* inputFiler() const;
	//wbblock
	PyDbDatabase wblock(const PyDbObjectIdArray& outObjIds, const AcGePoint3d& basePoint);

	PyDbDatabase wblock(const PyDbObjectIdArray& outObjIds, const AcGePoint3d& basePoint, AcDb::DuplicateRecordCloning drc);

	PyDbDatabase wblock(AcDbObjectId blockId);
	PyDbDatabase wblock();
	void              forceWblockDatabaseCopy();

	AcDbObjectId insert(AcString pBlockName, PyDbDatabase& pDb, bool preserveSourceDatabase);

	void insert(const AcGeMatrix3d& xform, PyDbDatabase& pDb, bool preserveSourceDatabase);

	AcDbObjectId insert(AcString pSourceBlockName, AcString pDestinationBlockName, PyDbDatabase& pDb, bool preserveSourceDatabase);

	void deepCloneObjects(const PyDbObjectIdArray& objectIds, const AcDbObjectId& owner, AcDbIdMapping& idMap, bool deferXlation);

	void wblockCloneObjects(const PyDbObjectIdArray& objectIds, const AcDbObjectId& owner, AcDbIdMapping& idMap, AcDb::DuplicateRecordCloning drc, bool deferXlation);
	void abortDeepClone(AcDbIdMapping& idMap);
	bool registerApp(const AcString& pszAppName);
	bool isAppRegistered(const AcString& pszAppName) const;
	Acad::ErrorStatus auditXData(AcDbAuditInfo* pInfo);

	AcDbUndoController* undoController() const;
	bool  disableUndoRecording(bool disable);
	bool              undoRecording() const;
	AcDbTransactionManager* transactionManager() const;

	AcDb::MaintenanceReleaseVersion maintenanceReleaseVersion() const;
	Acad::ErrorStatus restoreOriginalXrefSymbols();
	Acad::ErrorStatus restoreForwardingXrefSymbols();
	AcDbObjectId xrefBlockId() const;
	bool dimaso() const;          // 获取标注关联状态 (DIMASO)
	bool setDimaso(bool val);              // 设置标注关联性 (true=关联)

	bool dimsho() const;          // 获取实时更新状态 (DIMSHO)
	bool setDimsho(bool val);              // 设置动态更新 (true=启用)

	//=== 绘图模式控制 ===// 
	bool plinegen() const;        // 获取多段线顶点生成状态 (PLINEGEN)
	bool setPlinegen(bool val);            // 设置顶点生成 (true=启用)

	bool orthomode() const;       // 获取正交模式状态 (ORTHOMODE)
	bool setOrthomode(bool val);           // 设置正交模式 (true=启用)
	bool regenmode() const;       // 获取重生成模式状态 (REGENMODE)
	bool setRegenmode(bool val);           // 设置重生成模式 (true=自动重生成)

	bool fillmode() const;        // 获取填充显示状态 (FILLMODE)
	bool setFillmode(bool val);            // 设置填充显示 (true=显示填充)

	bool qtextmode() const;       // 获取快速文字模式状态 (QTEXTMODE)
	bool setQtextmode(bool val);           // 设置快速文字模式 (true=启用)

	bool psltscale() const;       // 获取线型比例状态 (PSLTSCALE)
	bool setPsltscale(bool val);           // 设置线型比例 (true=视口比例影响线型)
	//=== 绘图限制与显示控制 ===//
	bool limcheck() const;        // 获取绘图限制检查状态 (LIMCHECK)
	bool setLimcheck(bool val);            // 设置绘图限制检查 (true=启用)

	bool blipmode() const;        // 获取临时标记显示状态 (BLIPMODE)
	bool setBlipmode(bool val);            // 设置临时标记显示 (true=显示)

	//=== 新增常用系统变量 ===//
	bool mirrtext() const;              // MIRRTEXT 镜像文字
	bool setMirrtext(bool val);
	int attmode() const;                // ATTMODE 属性显示模式
	bool setAttmode(int val);
	int osmode() const;                 // OSMODE 对象捕捉模式
	bool setOsmode(int val);
	int pickbox() const;                // PICKBOX 拾取框大小
	bool setPickbox(int val);
	bool grips() const;                 // GRIPS 夹点显示
	bool setGrips(bool val);
	int gripsize() const;               // GRIPSIZE 夹点大小
	bool setGripsize(int val);
	bool highlight() const;             // HIGHLIGHT 高亮显示
	bool setHighlight(bool val);
	bool textfill() const;              // TEXTFILL 文字填充
	bool setTextfill(bool val);
	double filletrad() const;           // FILLETRAD 圆角半径
	bool setFilletrad(double val);
	double chamfera() const;            // CHAMFERA 倒角距离A
	bool setChamfera(double val);
	double chamferb() const;            // CHAMFERB 倒角距离B
	bool setChamferb(double val);
	double celtscale() const;           // CELTSCALE 当前线型比例
	bool setCeltscale(double val);
	int pdmode() const;                 // PDMODE 点样式
	bool setPdmode(int val);

	//=== 对象存储控制 ===//
	Adesk::Int16 saveproxygraphics() const;  // 获取代理图形保存级别
	void setSaveproxygraphics(Adesk::Int16 val);      // 设置代理图形保存级别

	Adesk::Int16 delUsedObj() const;         // 获取已删除对象处理方式 
	void setDelUsedObj(Adesk::Int16 val);             // 设置已删除对象处理方式 

	//=== 三维显示控制 ===//
	bool dispSilh() const;        // 获取轮廓线显示状态 (DISPSILH)
	void setDispSilh(bool val);            // 设置轮廓线显示 (true=启用)

	//=== 对象转换控制 ===//
	bool plineEllipse() const;    // 获取多段线椭圆转换状态 (PLINEELLIPSE)
	void setPlineEllipse(bool val);        // 设置多段线椭圆转换 (true=椭圆转为多段线)

	//=== 用户交互控制 ===//
	bool usrtimer() const;        // 获取用户计时器状态 (USRTIMER)
	void setUsrtimer(bool val);            // 设置用户计时器 (true=启用)

	//=== 绘图模式控制 ===//
	bool skpoly() const;          // 获取草图多段线模式 (SKPOLY)
	void setSkpoly(bool val);              // 设置草图模式 (true=生成多段线)

	bool angdir() const;          // 获取角度方向 (ANGDIR)
	void setAngdir(bool val);              // 设置角度方向 (true=顺时针)

	// 样条框架显示控制（SPLFRAME）
	bool splframe() const;
	void setSplframe(bool val);

	// 剪裁框架显示控制（XCLIPFRAME）
	Adesk::UInt8 xclipFrame() const;
	void setXclipFrame(Adesk::UInt8 val);

	// 属性相关控制
	bool attreq() const;
	void setAttreq(bool val);
	bool attdia() const;
	void setAttdia(bool val);

	// 对象操作控制

	// 视图与空间控制
	bool worldview() const;
	void setWorldview(bool val);

	bool tilemode() const;
	bool setTilemode(bool val);

	// 其他系统变量
	Adesk::Int16 dragmode() const;
	void setDragmode(Adesk::Int16 val);


	// 空间索引深度控制（TREEDEPTH）
	Adesk::Int16 treedepth() const;
	void setTreedepth(Adesk::Int16 val);

	// 线性单位控制（LUNITS/LUPREC）
	Adesk::Int16 lunits() const;
	bool setLunits(Adesk::Int16 val); ;

	Adesk::Int16 luprec() const;
	bool setLuprec(Adesk::Int16 val);

	// 角度单位控制（AUNITS/AUPREC）
	Adesk::Int16 aunits() const {
		return impObj()->aunits();
	}
	bool setAunits(Adesk::Int16 val);

	Adesk::Int16 auprec() const;
	bool setAuprec(Adesk::Int16 val);

	// 点显示模式（PDMODE）
	Adesk::Int16 pdmode();
	void setPdmode(Adesk::Int16 val);
	// 选择集样式控制 
	Adesk::Int16 pickstyle() const;
	void setPickstyle(Adesk::Int16 val);
	// 用户自定义整型变量 
	Adesk::Int16 useri1() const;
	void setUseri1(Adesk::Int16 val);

	Adesk::Int16 useri2() const;
	void setUseri2(Adesk::Int16 val);

	Adesk::Int16 useri3() const;
	void setUseri3(Adesk::Int16 val);
	Adesk::Int16 useri4() const;
	void setUseri4(Adesk::Int16 val);
	Adesk::Int16 useri5() const;
	void setUseri5(Adesk::Int16 val);

	// 样条曲线控制 
	Adesk::Int16 splinesegs() const;
	void setSplinesegs(Adesk::Int16 val);

	// 曲面控制
	Adesk::Int16 surfu() const;
	void setSurfu(Adesk::Int16 val);

	Adesk::Int16 surfv() const;
	void setSurfv(Adesk::Int16 val);

	Adesk::Int16 surftype() const;
	void setSurftype(Adesk::Int16 val);

	Adesk::Int16 surftab1() const;
	void setSurftab1(Adesk::Int16 val);

	Adesk::Int16 surftab2() const;
	void setSurftab2(Adesk::Int16 val);

	Adesk::Int16 splinetype() const;
	void setSplinetype(Adesk::Int16 val);

	// 着色控制 
	Adesk::Int16 shadedge() const;
	void setShadedge(Adesk::Int16 val);

	Adesk::Int16 shadedif() const;
	void setShadedif(Adesk::Int16 val);

	// 单位系统控制 
	AcDb::MeasurementValue measurement();
	void setMeasurement(AcDb::MeasurementValue val);

	Adesk::Int16 unitmode() const;

	void setUnitmode(Adesk::Int16 val);

	// 视口控制
	Adesk::Int16 maxactvp() const;
	void setMaxactvp(Adesk::Int16 val);
	Adesk::Int16 isolines() const;
	void setIsolines(Adesk::Int16 isolines);

	Adesk::UInt8 dwfframe() const;
	void setDwfframe(Adesk::UInt8 dwfframe);

	Adesk::UInt8 dgnframe() const;

	void setDgnframe(Adesk::UInt8 dgnframe);

	Adesk::Int8 pdfframe() const;
	void setPdfframe(Adesk::Int8 val);

	double ltscale() const;
	bool setLtscale(double scale);
	double textsize() const;
	bool setTextsize(double size);

	double tracewid() const;
	void setTracewid(double width);

	double sketchinc() const;
	void setSketchinc(double inc);


	double thickness() const;
	bool setThickness(double thickness);

	AcDb::LineWeight celweight() const;
	void setCelweight(AcDb::LineWeight weight);
	bool isValidLineWeight(int weight);

	AcDb::LineWeight getNearestLineWeight(int weight);

	AcDb::EndCaps endCaps() const;
	void setEndCaps(AcDb::EndCaps endCapstype);

	AcDb::JoinStyle joinStyle() const;
	void setJoinStyle(AcDb::JoinStyle style);

	bool lineWeightDisplay() const;
	void setLineWeightDisplay(bool display);

	bool xrefEditEnabled() const;
	void setXrefEditEnabled(bool enable);

	bool oleStartUp() const;
	void setOleStartUp(bool val);

	double angbase() const;
	bool setAngbase(double angle);

	double pdsize() const;
	bool setPdsize(double size);

	double plinewid() const;
	bool setPlinewid(double width);

	double userr1() const;
	bool setUserr1(double val);

	double userr2() const;
	bool setUserr2(double val);

	double userr3() const;
	void setUserr3(double val);

	double userr4() const;
	void setUserr4(double val);
	double userr5() const;
	void setUserr5(double val);



	double chamferc() const;
	void setChamferc(double val);
	double chamferd() const;
	void setChamferd(double val);

	double facetres() const;

	void setFacetres(double facetres);

	// 3DDWF
	double get3dDwfPrec() const;
	void set3dDwfPrec(double DwfPrec);

	AcString getMenu() const;



	bool hpInherit() const;
	void setHpInherit(const bool inherit);

	AcGePoint2d hpOrigin() const;
	void setHpOrigin(const AcGePoint2d& origin);

	// returns a local time
	const AcDbDate tdcreate() const;
	const AcDbDate tdupdate() const;

	const AcDbDate tducreate() const;

	const AcDbDate tduupdate() const;


	// returns a time delta
	const AcDbDate tdindwg() const;

	const AcDbDate tdusrtimer() const;

	void resetTimes();

	AcCmColor cecolor() const;
	void setCecolor(const AcCmColor& color);

	AcCmTransparency cetransparency() const;
	void setCetransparency(const AcCmTransparency& transparency);

	AcDbHandle handseed() const;
	void setHandseed(const AcDbHandle& handle);


	AcDbObjectId clayer() const;


	bool setClayer(AcDbObjectId objId);

	std::tuple<AcDb::PlotStyleNameType, AcDbObjectId> getCePlotStyleNameId() const;
	void setCePlotStyleName(AcDb::PlotStyleNameType plotsStyleNameType, AcDbObjectId newId = AcDbObjectId::kNull);


	AcDbObjectId textstyle() const;
	bool setTextstyle(AcDbObjectId objId);
	AcDbObjectId celtype() const;

	bool setCeltype(AcDbObjectId objId);

	AcDbObjectId cmaterial() const;
	bool setCmaterial(AcDbObjectId objId);

	AcDbObjectId dimstyle() const;
	bool setDimstyle(AcDbObjectId styleId);

	AcDbDimStyleTableRecord* getDimstyleData() const;

	AcDbDimStyleTableRecord* getDimstyleChildData(const PyRxClass& pDimClass, AcDbObjectId& style) const;
	AcDbObjectId getDimstyleChildId(const PyRxClass& pDimClass, AcDbObjectId& parentStyle) const;
	AcDbObjectId getDimstyleParentId(AcDbObjectId& childStyle) const;

	void setDimstyleData(AcDbDimStyleTableRecord* pRec);
	void setDimstyleData(AcDbObjectId id);

	AcDbObjectId cmlstyleID() const;
	void setCmlstyleID(AcDbObjectId objId);

	Adesk::Int16 cmljust() const;
	void setCmljust(Adesk::Int16 just);

	double cmlscale() const;
	void setCmlscale(double scale);


	// Paper space variable access

	AcGePoint3d pinsbase() const;
	void setPinsbase(const AcGePoint3d& base);

	AcGePoint3d pextmin() const;

	AcGePoint3d pextmax() const;

	void setPextmin(const AcGePoint3d& min);
	void setPextmax(const AcGePoint3d& max);
	AcGePoint2d plimmin() const;
	void setPlimmin(const AcGePoint2d& min);

	AcGePoint2d plimmax() const;
	void setPlimmax(const AcGePoint2d& max);

	double pelevation() const;
	void setPelevation(double elev);

	AcGePoint3d pucsorg() const;
	AcGeVector3d pucsxdir() const;
	AcGeVector3d pucsydir() const;
	void setPucs(const AcGePoint3d& ucsOrigin, const AcGeVector3d& ucsXDir, const AcGeVector3d& ucsYDir);

	AcDbObjectId pucsname() const;

	void setPucsname(const AcDbObjectId& ucsRecId);
	AcDbObjectId pucsBase() const;

	void setPucsBase(const AcDbObjectId& ucsid);

	AcGePoint3d worldPucsBaseOrigin(AcDb::OrthographicView orthoView) const;
	void setWorldPucsBaseOrigin(const AcGePoint3d& origin, AcDb::OrthographicView orthoView);

	bool isPucsOrthographic(AcDb::OrthographicView& orthoView) const;

	double cameraHeight() const;

	void setCameraHeight(const double cameraHeight);

	double lensLength() const;
	void setLensLength(const double lensLength);

	bool cameraDisplay() const;
	void setCameraDisplay(const bool cameraDisplay);

	double stepsPerSec() const;
	void setStepsPerSec(double stepsPerSec);

	double stepSize() const;
	void setStepSize(double stepSize);

	bool realWorldScale() const;
	void setRealWorldScale(const bool realWorldScale);

	AcGePoint3d insbase() const;
	void setInsbase(const AcGePoint3d& base);

	AcGePoint3d extmin() const;

	AcGePoint3d extmax() const;
	bool updateExt(bool doBestFit = false);

	void setExtmin(const AcGePoint3d& min);
	void setExtmax(const AcGePoint3d& max);

	AcGePoint2d limmin() const;
	void setLimmin(const AcGePoint2d& min);

	AcGePoint2d limmax() const;
	void setLimmax(const AcGePoint2d& max);

	double elevation() const;
	bool setElevation(double elev);

	AcGePoint3d ucsorg() const;

	AcGeVector3d ucsxdir() const;

	AcGeVector3d ucsydir() const;

	void setUcs(const AcGePoint3d& ucsOrigin, const AcGeVector3d& ucsXDir, const AcGeVector3d& ucsYDir);

	AcDbObjectId ucsname() const;
	void setUcsname(const AcDbObjectId& ucsRecId);

	AcDbObjectId ucsBase() const;
	void setUcsBase(AcDbObjectId ucsid);

	AcGePoint3d worldUcsBaseOrigin(AcDb::OrthographicView orthoView) const;
	void setWorldUcsBaseOrigin(const AcGePoint3d& origin, AcDb::OrthographicView orthoView);

	bool isUcsOrthographic(AcDb::OrthographicView& orthoView) const;

	bool isEMR() const;
	AcString getDimpost() const;
	AcString getDimapost() const;
	AcString getDimblk() const;

	AcString getDimblk1() const;

	AcString getDimblk2() const;

	int dimfit() const;
	int dimunit() const;

	void setDimfit(int fit);
	void setDimunit(int unit);
	//
	// end DEPRECATED METHODS!


	void getDimRecentStyleList(PyDbObjectIdArray& objIds) const;

	void loadLineTypeFile(AcString ltn, AcString filename);

	AcString getProjectName() const;
	bool setProjectName(AcString val);

	AcString getHyperlinkBase() const;
	bool setHyperlinkBase(AcString val);

	AcString getStyleSheet() const;
	void setStyleSheet(AcString);

	AcString getFilename() const;

	bool isPartiallyOpened() const;
	void applyPartialOpenFilters(const AcDbSpatialFilter* pSpatialFilter, const AcDbLayerFilter* pLayerFilter);
	void disablePartialOpen();

	AcString getFingerprintGuid() const;
	void setFingerprintGuid(AcString val);

	AcString getVersionGuid() const;
	void setVersionGuid(AcString val);


	// New text sysvars
	//
	int tstackalign() const;
	void setTStackAlign(int val);

	int tstacksize() const;
	void setTStackSize(int val);

	AcDb::UnitsValue insunits() const;
	bool setInsunits(const AcDb::UnitsValue units);


	// Plot Style Legacy
	bool plotStyleMode() const;

	// Default paperspace viewport scale
	double viewportScaleDefault() const;
	void setViewportScaleDefault(double newDefaultVPScale);

	// dwg watermark
	bool dwgFileWasSavedByAutodeskSoftware() const;

	// Layer state manager API
	AcDbLayerStateManager* getLayerStateManager() const;

	// Object context manager API
	AcDbObjectContextManager* objectContextManager() const;

	// When entity traversals are sorted (AcDb::kSortEnts*).
	Adesk::UInt8 sortEnts() const;
	void setSortEnts(Adesk::UInt8 sortEnts);

	Adesk::UInt8 drawOrderCtl() const;
	void setDrawOrderCtl(Adesk::UInt8 val);

	Adesk::UInt8 dimAssoc() const;
	void setDimAssoc(Adesk::UInt8 val);

	Adesk::UInt8 hideText() const;
	void setHideText(Adesk::UInt8 val);

	Adesk::UInt8 haloGap() const;
	void setHaloGap(Adesk::UInt8 val);

	Adesk::UInt16 obscuredColor() const;
	void setObscuredColor(Adesk::UInt16 val);

	Adesk::UInt8 obscuredLineType() const;
	void setObscuredLineType(Adesk::UInt8 val);

	Adesk::UInt8 intersectDisplay() const;
	void setIntersectDisplay(Adesk::UInt8 val);

	Adesk::UInt16 intersectColor() const;
	void setIntersectColor(Adesk::UInt16 val);

	// For the ACAD_TABLESTYLE dictionary
	void getTableStyleDictionary(AcDbDictionary*& pDict, AcDb::OpenMode mode = AcDb::kForWrite);
	AcDbObjectId tableStyleDictionaryId() const;

	// The following functions are similar to setDimstyle/dimstyle
	//
	AcDbObjectId tablestyle() const;
	void setTablestyle(AcDbObjectId objId);

	// For the ACAD_MLEADERSTYLE dictionary
	AcDbDictionary* getMLeaderStyleDictionary(AcDb::OpenMode mode);
	AcDbObjectId mleaderStyleDictionaryId() const;



	// The following functions are similar to setDimstyle/dimstyle
	//
	AcDbObjectId mleaderstyle() const;
	void setMLeaderstyle(AcDbObjectId objId);

	// For the ACAD_DETAILVIEWSTYLE dictionary

	AcDbDictionary* getDetailViewStyleDictionary(AcDb::OpenMode mode);
	AcDbObjectId detailViewStyleDictionaryId() const;
	AcDbObjectId detailViewStyle() const;
	//Do not change format of setDetailViewStyle function! 
	//We need to preserve format which SymbolTableInfo struct defines for AcDiObservableCollection datatype (AcDiImpManager object)
	void setDetailViewStyle(AcDbObjectId objId);

	// For the ACAD_SECTIONVIEWSTYLE dictionary
	AcDbDictionary* getSectionViewStyleDictionary(AcDb::OpenMode mode);
	AcDbObjectId sectionViewStyleDictionaryId() const;
	AcDbObjectId sectionViewStyle() const;
	//Do not change format of setSectionViewStyle function! 
	//We need to preserve format which SymbolTableInfo struct defines for AcDiObservableCollection datatype (AcDiImpManager object)
	void setSectionViewStyle(AcDbObjectId objId);
	PyDbObjectIdArray getViewportArray(bool bGetPaperspaceVports) const;
	// Get names of all visual styles in the database (visual styles for
	// internal use only are excluded)
	std::vector<AcString> getVisualStyleList();

	Adesk::UInt8 solidHist() const; // 0 (default) or 1
	void setSolidHist(Adesk::UInt8 val);

	Adesk::UInt8 showHist() const; // 0, 1(default) or 2
	void setShowHist(Adesk::UInt8 val);

	double psolWidth() const;
	void setPsolWidth(double width);
	double psolHeight() const;
	void setPsolHeight(double height);

	Adesk::UInt16 loftParam() const;
	void setLoftParam(Adesk::UInt16 flags);
	Adesk::UInt8 loftNormals() const;
	void setLoftNormals(Adesk::UInt8 value);
	double loftAng1() const;
	void setLoftAng1(double ang1);
	double loftAng2() const;
	void setLoftAng2(double ang2);
	double loftMag1() const;
	void setLoftMag1(double mag1);
	double loftMag2() const;
	void setLoftMag2(double mag2);

	// Lat and long are in decimal degrees, not radians!
	double latitude() const;
	void setLatitude(double lat);
	double longitude() const;
	void setLongitude(double lng);
	double northDirection() const;
	void setNorthDirection(double northdir);
	AcDb::TimeZone timeZone() const;
	void setTimeZone(AcDb::TimeZone tz);
	// Offsets measured in hours difference from UTC
	void getTimeZoneInfo(AcDb::TimeZone tz, double& offset, AcString& desc) const;
	void setTimeZoneAsUtcOffset(double offset);
	bool geoMarkerVisibility() const;
	void setGeoMarkerVisibility(bool value);
	AcString geoCoordinateSystemId() const;

	Adesk::UInt8 lightGlyphDisplay() const; // 0 or 1(default)
	void setLightGlyphDisplay(Adesk::UInt8 val);
	Adesk::UInt8 tileModeLightSynch() const; // 0 or 1(default)
	void setTileModeLightSynch(Adesk::UInt8 val);

	AcCmColor interfereColor() const;
	void setInterfereColor(const AcCmColor& color);

	AcDbObjectId interfereObjVisStyle() const;
	void setInterfereObjVisStyle(AcDbObjectId id);

	AcDbObjectId interfereVpVisStyle() const;
	void setInterfereVpVisStyle(AcDbObjectId id);

	AcDbObjectId dragVisStyle() const;
	void setDragVisStyle(AcDbObjectId id);

	Adesk::UInt8 cshadow() const;
	void setCshadow(Adesk::UInt8 val);

	double shadowPlaneLocation() const;
	void setShadowPlaneLocation(double val);

	AcDbAnnotationScale* cannoscale() const;
	void setCannoscale(AcDbAnnotationScale*);

	bool annoAllVisible() const;
	void setAnnoAllVisible(bool allvis);

	bool annotativeDwg() const;
	void setAnnotativeDwg(bool anno);

	bool msltscale() const;
	void setMsltscale(bool val);

	Adesk::UInt8 lightingUnits() const;
	void setLightingUnits(Adesk::UInt8 val);

	bool lightsInBlocks() const;
	void setLightsInBlocks(bool val);

	Adesk::Int16 dxeVal() const;
	void setDxeVal(Adesk::Int16 val);

	double mleaderscale() const;
	void setMLeaderscale(double scale);

	Adesk::Int16 cDynDisplayMode() const;
	void setCDynDisplayMode(Adesk::Int16 val);

	Adesk::Int16 previewType() const;
	void setPreviewType(Adesk::Int16 val);

	Adesk::UInt8 indexctl() const;
	void setIndexctl(Adesk::UInt8 val);

	Adesk::UInt8 layerEval() const;
	void setLayerEval(Adesk::UInt8 val);

	Adesk::Int16 layerNotify() const;
	void setLayerNotify(Adesk::Int16 val);

	double msOleScale() const;
	void setMsOleScale(double val);

	Adesk::UInt8 updateThumbnail() const;
	void setUpdateThumbnail(Adesk::UInt8 val);

	Adesk::UInt32 eraseEmptyObjects(const Adesk::UInt32 flags);
	Adesk::UInt32 countEmptyObjects(const Adesk::UInt32 flags);

};


enum FindTextOptions:uint8_t {
	kSearchBlock = 0x01,
	kSearchDimText = 0x02,
	kSearchText = 0x04,
	kSearchLinkDesc = 0x08,
	kSearchLinkUrl = 0x10,
	kSearchMatchCase = 0x20,
	kSearchWholeWord = 0x40,
	kSearchDefault = 0x1F
};

PyDbObjectIdArray PyDbTextFind(PyDbDatabase& pDatabase,
	PyDbObjectIdArray& resultSet,
	AcString findString,
	AcString replaceString,
	FindTextOptions searchOptions = FindTextOptions::kSearchDefault,
	const PyDbObjectIdArray& selSet = PyDbObjectIdArray());

bool IsGsWorkerThread();



void bind_PyDbDatabase(pym& m);
