def _decode_header_vars(container: Container) -> Dict[str, object]:
    """Decode AcDb:Header for AC1021, every variable in one bit walk.

    Field order, codes and the R2007-only branches follow the ACadSharp
    ``DwgHeaderReader`` R2007 path (MIT reference, independent implementation).
    Three streams are involved, exactly as the reference reader sets them up:
      * main  — every numeric field plus HANDSEED (a handle reference that lives
                inline in the main stream, not in the handle stream);
      * text  — every TV string, located by back-shifting from the flag bit at
                the end of the main stream;
      * handle— every H reference, in field order.
    ``closed.delta == 0`` means the walk consumed the main stream bit-exactly
    up to the text-stream start, i.e. the field list is the real one.
    """
    from .objects import Reader as StringReader

    result: Dict[str, object] = {
        "codepage": container.codepage,
        "vars": {},
        "handles": {},
        "insunits": None,
        "extmin": None,
        "extmax": None,
        "handseed": None,
        "block_record_paper_space": None,
        "block_record_model_space": None,
        "insbase": None,
        "limmin": None,
        "limmax": None,
        "status": {},
    }
    section = container.sections.get("AcDb:Header")
    if section is None:
        result["status"]["section"] = "missing"
        return result

    data = section.data
    try:
        pre = BitReader(data, 128, what="header preamble")
        pre.rl()  # declared section size in bytes
        size_in_bits = pre.rl()
        start = pre.pos
        last = start + size_in_bits - 1
        body_end = (len(data) - 18) * 8  # 2-byte CRC + 16-byte end sentinel
    except Exception as exc:
        result["status"]["preamble"] = str(exc)
        return result

    text = _locate_text_stream(data, start, last)
    main_end = text[0] if text else last
    main = BitReader(data, start, main_end, what="header main stream")
    hdl = BitReader(data, last + 1, body_end, what="header handle stream")
    strings = StringReader(data, text[0], text[1]) if text else None

    values: Dict[str, object] = {}
    handles: Dict[str, object] = {}
    unknown: List[str] = []

    def b(name: str) -> int:
        value = main.bit()
        values[name] = value
        return value

    def bs(name: str) -> int:
        value = main.bs()
        values[name] = value
        return value

    def bl(name: str) -> int:
        value = main.bl()
        values[name] = value
        return value

    def rc(name: str) -> int:
        value = main.rc()
        values[name] = value
        return value

    def bd(name: str) -> float:
        value = main.bd()
        values[name] = value
        return value

    def bd3(name: str) -> List[float]:
        value = main.bd3()
        values[name] = value
        return value

    def rd2(name: str) -> List[float]:
        value = main.rd2()
        values[name] = value
        return value

    def tv(name: str) -> str:
        value = strings.tv() if strings is not None else ""
        values[name] = value
        return value

    def cmc(name: str) -> None:
        index = main.bs()
        rgb = main.bl()
        flags = main.rc()
        color_name = strings.tv() if (strings is not None and flags & 1) else ""
        book_name = strings.tv() if (strings is not None and flags & 2) else ""
        if index and rgb == 0xC0000000:
            aci = 256
        elif rgb & 0x100:
            aci = rgb & 0xFF
        else:
            aci = None
        values[name] = {"aci": aci, "rgb": rgb, "flags": flags,
                        "name": color_name, "book": book_name}

    def hh(name: str) -> int:
        code, value = hdl.handle()
        values[name] = value
        handles[name] = {"code": code, "value": value}
        return value

    def b_unk(tag: str) -> None:
        main.bit()
        unknown.append(tag)

    def bs_unk(tag: str) -> None:
        main.bs()
        unknown.append(tag)

    def bl_unk(tag: str) -> None:
        main.bl()
        unknown.append(tag)

    def bd_unk(tag: str) -> None:
        main.bd()
        unknown.append(tag)

    def rc_unk(tag: str) -> None:
        main.rc()
        unknown.append(tag)

    def tv_unk(tag: str) -> None:
        if strings is not None:
            strings.tv()
        unknown.append(tag)

    try:
        for i in range(4):
            bd_unk(f"unknown_bd{i + 1}")
        for i in range(4):
            tv_unk(f"unknown_tv{i + 1}")
        bl_unk("unknown_bl1")
        bl_unk("unknown_bl2")

        b("DIMASO")
        b("DIMSHO")
        for name in ("PLINEGEN", "ORTHOMODE", "REGENMODE", "FILLMODE",
                     "QTEXTMODE", "PSLTSCALE", "LIMCHECK"):
            b(name)
        b_unk("unknown_r2004_b")
        for name in ("USRTIMER", "SKPOLY", "ANGDIR", "SPLFRAME"):
            b(name)
        b("MIRRTEXT")
        b("WORLDVIEW")
        for name in ("TILEMODE", "PLIMCHECK", "VISRETAIN"):
            b(name)
        b("DISPSILH")
        b("PELLIPSE")

        bs("PROXYGRAPHICS")
        for name in ("TREEDEPTH", "LUNITS", "LUPREC", "AUNITS", "AUPREC"):
            bs(name)
        bs("ATTMODE")
        bs("PDMODE")
        for i in range(3):
            bl_unk(f"unknown_bl_r2004_{i + 1}")
        for name in (
            "USERI1", "USERI2", "USERI3", "USERI4", "USERI5",
            "SPLINESEGS", "SURFU", "SURFV", "SURFTYPE", "SURFTAB1",
            "SURFTAB2", "SPLINETYPE", "SHADEDGE", "SHADEDIF", "UNITMODE",
            "MAXACTVP", "ISOLINES", "CMLJUST", "TEXTQLTY",
        ):
            bs(name)
        for name in (
            "LTSCALE", "TEXTSIZE", "TRACEWID", "SKETCHINC", "FILLETRAD",
            "THICKNESS", "ANGBASE", "PDSIZE", "PLINEWID", "USERR1", "USERR2",
            "USERR3", "USERR4", "USERR5", "CHAMFERA", "CHAMFERB", "CHAMFERC",
            "CHAMFERD", "FACETRES", "CMLSCALE", "CELTSCALE",
        ):
            bd(name)
        tv("MENUNAME")

        # TDCREATE / TDUPDATE: Julian day + milliseconds inside the day.
        for name in ("TDCREATE", "TDUPDATE"):
            day = bl(f"{name}_day")
            ms = bl(f"{name}_ms")
            values[name] = day + ms / 86400000.0
        for i in range(3):
            bl_unk(f"unknown_bl_r2004_{i + 4}")
        for name in ("TDINDWG", "TDUSRTIMER"):
            days = bl(f"{name}_days")
            ms = bl(f"{name}_ms")
            values[name] = days + ms / 86400000.0

        cmc("CECOLOR")

        # HANDSEED is inline in the main stream (not part of the handle stream).
        seed_code, seed_value = main.handle()
        values["HANDSEED"] = seed_value
        handles["HANDSEED"] = {"code": seed_code, "value": seed_value}

        for name in ("CLAYER", "TEXTSTYLE", "CELTYPE", "CMATERIAL",
                     "DIMSTYLE", "CMLSTYLE"):
            hh(name)

        bd("PSVPSCALE")
        bd3("PINSBASE")
        bd3("PEXTMIN")
        bd3("PEXTMAX")
        rd2("PLIMMIN")
        rd2("PLIMMAX")
        bd("PELEVATION")
        bd3("PUCSORG")
        bd3("PUCSXDIR")
        bd3("PUCSYDIR")
        hh("PUCSNAME")
        hh("PUCSORTHOREF")
        bs("PUCSORTHOVIEW")
        hh("PUCSBASE")
        for name in ("PUCSORGTOP", "PUCSORGBOTTOM", "PUCSORGLEFT",
                     "PUCSORGRIGHT", "PUCSORGFRONT", "PUCSORGBACK"):
            bd3(name)

        bd3("INSBASE")
        bd3("EXTMIN")
        bd3("EXTMAX")
        rd2("LIMMIN")
        rd2("LIMMAX")
        bd("ELEVATION")
        bd3("UCSORG")
        bd3("UCSXDIR")
        bd3("UCSYDIR")
        hh("UCSNAME")
        hh("UCSORTHOREF")
        bs("UCSORTHOVIEW")
        hh("UCSBASE")
        for name in ("UCSORGTOP", "UCSORGBOTTOM", "UCSORGLEFT",
                     "UCSORGRIGHT", "UCSORGFRONT", "UCSORGBACK"):
            bd3(name)
        tv("DIMPOST")
        tv("DIMAPOST")

        for name in ("DIMSCALE", "DIMASZ", "DIMEXO", "DIMDLI", "DIMEXE",
                     "DIMRND", "DIMDLE", "DIMTP", "DIMTM"):
            bd(name)
        bd("DIMFXL")
        bd("DIMJOGANG")
        bs("DIMTFILL")
        cmc("DIMTFILLCLR")
        for name in ("DIMTOL", "DIMLIM", "DIMTIH", "DIMTOH", "DIMSE1", "DIMSE2"):
            b(name)
        for name in ("DIMTAD", "DIMZIN", "DIMAZIN"):
            bs(name)
        bs("DIMARCSYM")
        for name in ("DIMTXT", "DIMCEN", "DIMTSZ", "DIMALTF", "DIMLFAC",
                     "DIMTVP", "DIMTFAC", "DIMGAP"):
            bd(name)
        bd("DIMALTRND")
        b("DIMALT")
        bs("DIMALTD")
        for name in ("DIMTOFL", "DIMSAH", "DIMTIX", "DIMSOXD"):
            b(name)
        for name in ("DIMCLRD", "DIMCLRE", "DIMCLRT"):
            cmc(name)
        for name in ("DIMADEC", "DIMDEC", "DIMTDEC", "DIMALTU", "DIMALTTD",
                     "DIMAUNIT", "DIMFRAC", "DIMLUNIT", "DIMDSEP", "DIMTMOVE",
                     "DIMJUST"):
            bs(name)
        b("DIMSD1")
        b("DIMSD2")
        for name in ("DIMTOLJ", "DIMTZIN", "DIMALTZ", "DIMALTTZ"):
            bs(name)
        b("DIMUPT")
        bs("DIMATFIT")
        b("DIMFXLON")
        for name in ("DIMTXSTY", "DIMLDRBLK", "DIMBLK", "DIMBLK1", "DIMBLK2"):
            hh(name)
        for name in ("DIMLTYPE", "DIMLTEX1", "DIMLTEX2"):
            hh(name)
        for name in ("DIMLWD", "DIMLWE"):
            bs(name)

        for name in ("BLOCK_CONTROL", "LAYER_CONTROL", "STYLE_CONTROL",
                     "LINETYPE_CONTROL", "VIEW_CONTROL", "UCS_CONTROL",
                     "VPORT_CONTROL", "APPID_CONTROL", "DIMSTYLE_CONTROL"):
            hh(name)
        for name in ("DICT_GROUP", "DICT_MLINESTYLE", "DICT_NAMED_OBJECTS"):
            hh(name)
        bs("TSTACKALIGN")
        bs("TSTACKSIZE")
        tv("HYPERLINKBASE")
        tv("STYLESHEET")
        for name in ("DICT_LAYOUTS", "DICT_PLOTSETTINGS", "DICT_PLOTSTYLES",
                     "DICT_MATERIALS", "DICT_COLORS", "DICT_VISUALSTYLE"):
            hh(name)

        bl("FLAGS")
        flags = int(values["FLAGS"])
        values["CELWEIGHT"] = flags & 0x1F
        values["ENDCAPS"] = flags & 0x60
        values["JOINSTYLE"] = flags & 0x180
        values["LWDISPLAY"] = 1 if flags & 0x200 else 0
        values["XEDIT"] = 1 if flags & 0x400 else 0
        values["EXTNAMES"] = 1 if flags & 0x800 else 0
        values["PSTYLEMODE"] = 1 if flags & 0x2000 else 0
        values["OLESTARTUP"] = 1 if flags & 0x4000 else 0
        bs("INSUNITS")
        cepsntype = bs("CEPSNTYPE")
        if cepsntype == 3:
            hh("CPSNID")
        tv("FINGERPRINTGUID")
        tv("VERSIONGUID")

        for name in ("SORTENTS", "INDEXCTL", "HIDETEXT", "XCLIPFRAME",
                     "DIMASSOC", "HALOGAP"):
            rc(name)
        bs("OBSCUREDCOLOR")
        bs("INTERSECTIONCOLOR")
        for name in ("OBSCUREDLTYPE", "INTERSECTIONDISPLAY"):
            rc(name)
        tv("PROJECTNAME")

        hh("PAPER_SPACE")
        hh("MODEL_SPACE")
        hh("BYLAYER")
        hh("BYBLOCK")
        hh("CONTINUOUS")

        b("CAMERADISPLAY")
        bl_unk("unknown_bl_camera1")
        bl_unk("unknown_bl_camera2")
        bd_unk("unknown_bd_camera")
        for name in ("STEPSPERSEC", "STEPSIZE", "3DDWFPREC", "LENSLENGTH",
                     "CAMERAHEIGHT"):
            bd(name)
        for name in ("SOLIDHIST", "SHOWHIST"):
            rc(name)
        for name in ("PSOLWIDTH", "PSOLHEIGHT", "LOFTANG1", "LOFTANG2",
                     "LOFTMAG1", "LOFTMAG2"):
            bd(name)
        bs("LOFTPARAM")
        rc("LOFTNORMALS")
        for name in ("LATITUDE", "LONGITUDE", "NORTHDIRECTION"):
            bd(name)
        bl("TIMEZONE")
        for name in ("LIGHTGLYPHDISPLAY", "DWFFRAME", "DGNFRAME"):
            rc(name)
        rc_unk("unknown_rc_tilemodelightsynch")
        b_unk("unknown_b_r2007")
        cmc("INTERFERECOLOR")
        for name in ("INTERFEREOBJVS", "INTERFEREVPVS", "DRAGVS"):
            hh(name)
        rc("CSHADOW")
        bd("SHADOWPLANELOCATION")
    except (BitCodeError, DwgBoundsError, ValueError, IndexError) as exc:
        result["status"]["main_walk"] = str(exc)

    delta = main_end - main.pos
    result["status"]["closed"] = (
        "main.pos == text stream start (delta=0)" if delta == 0
        else f"delta={delta} bits (positive = stream left over)"
    )
    result["status"]["delta"] = delta
    result["status"]["handle_refs"] = hdl.pos - (last + 1)
    result["status"]["unknown_fields"] = len(unknown)

    result["vars"] = values
    result["handles"] = handles
    result["unknown"] = unknown
    # Legacy flat keys consumed by model/dump and the comparison tool.
    result["insunits"] = values.get("INSUNITS")
    result["insbase"] = values.get("INSBASE")
    result["extmin"] = values.get("EXTMIN")
    result["extmax"] = values.get("EXTMAX")
    result["limmin"] = values.get("LIMMIN")
    result["limmax"] = values.get("LIMMAX")
    result["handseed"] = values.get("HANDSEED")
    paper = handles.get("PAPER_SPACE") or {}
    model = handles.get("MODEL_SPACE") or {}
    if paper.get("value"):
        result["block_record_paper_space"] = f"{paper['value']:X}"
    if model.get("value"):
        result["block_record_model_space"] = f"{model['value']:X}"
    return result
