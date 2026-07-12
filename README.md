# 共轴球面光学系统光线追迹与像差计算程序

## 运行方法

**已编译版本直接运行（免编译）：**

```bash
# Web 图形界面
python3 ui/server.py                            
```

运行依赖：Python 3 + Flask（Web UI 需要）。


**注意：** `example_doublet_*.json` 中的折射率是占位符（`"PLEASE_FILL..."`），需替换为实际玻璃库数据后使用。`test_*.json` 包含 H-K9L / H-ZF2 的完整折射率，可直接运行。

## 输入文件格式（JSON）

`config.json` 指定运行模式和输入文件：

```json
{
  "mode": "compute76",
  "infinity": "examples/test_infinity.json",
  "finite": "examples/test_finite.json",
  "output": "outputs/result_76.txt"
}
```

Modes: `compute76`（无限远+有限距，76 项）或 `compute`（单配置，38 项）。

光学系统 JSON 完整示例：

```json
{
  "system_name": "H-K9L_H-ZF2_doublet",
  "object_at_infinity": true,
  "object_distance": null,
  "field_angle_deg": 3.0,
  "object_height": null,
  "entrance_pupil_position": 0.0,
  "entrance_pupil_diameter": 20.0,
  "stop_surface": 1,
  "field_ratios": [0.0, 0.7, 1.0],
  "aperture_ratios": [-1.0, -0.7, 0.0, 0.7, 1.0],
  "wavelength_types": ["d", "F", "C"],
  "materials": {
    "AIR":   {"nd": 1.0,     "nF": 1.0,     "nC": 1.0},
    "H-K9L": {"nd": 1.5168, "nF": 1.5224, "nC": 1.5143},
    "H-ZF2": {"nd": 1.6727, "nF": 1.6875, "nC": 1.6666}
  },
  "surfaces": [
    {"radius": 62.5,    "thickness": 4.0, "material_after": "H-K9L"},
    {"radius": -43.65,  "thickness": 2.5, "material_after": "H-ZF2"},
    {"radius": -124.35, "thickness": 0.0, "material_after": "AIR"}
  ]
}
```

### 顶层字段

| 字段 | 类型 | 说明 |
|---|---|---|
| `system_name` | 字符串 | 系统名称，仅标识 |
| `object_at_infinity` | 布尔 | `true`=无限远；`false`=有限距 |
| `object_distance` | 数值或 `null` | 物距（mm），实物填负数。无限远时 `null` |
| `field_angle_deg` | 数值或 `null` | 半视场角（°），无限远时必填。有限距时 `null` |
| `object_height` | 数值或 `null` | 物高（mm），有限距时必填。无限远时 `null` |
| `entrance_pupil_position` | 数值 | 入瞳距第一面顶点距离（mm） |
| `entrance_pupil_diameter` | 数值 | 入瞳直径（mm），正数 |
| `stop_surface` | 整数 | 光阑面号（从 1 计数） |
| `field_ratios` | 数组 | 视场比例 |
| `aperture_ratios` | 数组 | 孔径比例 |
| `wavelength_types` | 数组 | `"d"` / `"F"` / `"C"` |

### 材料与面型

`materials`：JSON 对象，key=材料名，包含 `nd`/`nF`/`nC` 三个折射率。必须含 `"AIR"` (折射率 1.0)。

`surfaces`：JSON 数组，每项含 `radius`（曲率半径，"inf" 表示平面）、`thickness`（到下一面的轴向距离）、`material_after`（面后介质名）。

## 符号规则

| 量 | 正方向 | 说明 |
|---|---|---|
| z 轴 | +z 光传播（左→右） | |
| 曲率半径 r | 球心在顶点右侧 | `inf` = 平面 |
| 物距 L | 交点右为正 | 实物 L < 0 |
| 光线角度 U | 逆时针 | tan(U)=dy/dz |
| 焦距 f' | 会聚为正 | f' = -h1/u'_k |
| 像高 | 负 = 光轴下方（倒像） | |
| 位置色差 | `L_F' - L_C'` | |

## 输出文件

固定宽度文本，5 列：

```
case      order  name_zh                              value           unit
--------  -----  ----------------------------------  ---------------  ----
infinity     1 焦距 f'                                99.718291 mm
...
finite      38 子午彗差，d 光，全视场，全孔径       0.046658 mm
```

compute76 先输出 infinity 38 项，再输出 finite 38 项，共 76 行。

## 与 Zemax 人工校对

1. 在 Zemax 建立相同系统（半径、厚度、材料、入瞳、视场完全一致）
2. 运行本程序 → `outputs/result_76.txt`
3. 先核对 `f_prime`、`l_prime` → 核对 `lp_prime` → 核对实际像高 → 核对各项像差
4. 本程序不自动与 Zemax 比较，需人工逐项比对

## 不支持的功能

非共轴系统、棱镜/反射面、非球面、光阑倒追、弧矢面追迹、三维光线、赛德尔系数、MTF/点列图/像差曲线、Zemax 文件读取、自动比较、优化设计。
