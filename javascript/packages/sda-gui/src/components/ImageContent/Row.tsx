import { useCallback } from 'react';
import { ImageRowType, ImageBaseRow, ImageInstructionRow } from 'sda-electron/api/image';
import { Group, Rect } from 'react-konva';
import { buildKonvaFormatText } from 'components/Konva';
import { StylesType } from './style';
import { useImageContent } from './context';

export interface ImageRowElement {
  offset: number;
  height: number;
  element: JSX.Element;
}

const buildInstructionRow = (row: ImageInstructionRow, style: StylesType): ImageRowElement => {
  const offsetText = buildKonvaFormatText({
    selectionArea: 'offset',
    textIdx: row.offset,
    textParts: [
      {
        text: `0x${row.offset.toString(16)}`,
        style: { fontSize: 12, fill: 'white' },
      },
    ],
    maxWidth: style.row.cols.instruction.width,
    newLineInEnd: true,
  });
  const instructionText = buildKonvaFormatText({
    selectionArea: 'instruction',
    textIdx: row.offset,
    textParts: row.tokens.map((token) => {
      return {
        text: token.text,
        style: {
          fontSize: 12,
          fill: style.row.instructionTokenColors[token.type] || 'white',
        },
      };
    }),
    maxWidth: style.row.cols.instruction.width,
    newLineInEnd: true,
  });
  const height = instructionText.height + style.row.padding * 2;
  function Elem() {
    const {
      rowSelection: { selectedRows, firstSelectedRow, setFirstSelectedRow, setLastSelectedRow },
    } = useImageContent();
    const rowWidth = style.row.width;
    const isSelected = selectedRows.includes(row.offset);

    const onMouseDown = useCallback(() => {
      setFirstSelectedRow?.(row.offset);
    }, [setFirstSelectedRow]);

    const onMouseMove = useCallback(() => {
      if (firstSelectedRow !== undefined) {
        setLastSelectedRow?.(row.offset);
      }
    }, [firstSelectedRow, setFirstSelectedRow]);

    const onMouseUp = useCallback(() => {
      setFirstSelectedRow?.(undefined);
      setLastSelectedRow?.(undefined);
    }, [setFirstSelectedRow, setLastSelectedRow]);

    return (
      <Group
        width={style.row.width}
        onMouseDown={onMouseDown}
        onMouseMove={onMouseMove}
        onMouseUp={onMouseUp}
      >
        <Rect width={rowWidth} height={height} fill={isSelected ? '#360b0b' : '#00000000'} />
        <Group x={style.row.padding} y={style.row.padding}>
          <Group width={style.row.cols.jump.width}></Group>
          <Group
            x={style.row.padding + style.row.cols.jump.width}
            width={style.row.cols.offset.width}
          >
            {offsetText.elem}
          </Group>
          <Group
            x={style.row.padding + style.row.cols.jump.width + style.row.cols.offset.width}
            width={style.row.cols.instruction.width}
          >
            {instructionText.elem}
          </Group>
        </Group>
      </Group>
    );
  }
  return {
    offset: row.offset,
    height,
    element: <Elem />,
  };
};

export const buildRow = (row: ImageBaseRow, styles: StylesType): ImageRowElement => {
  if (row.type === ImageRowType.Instruction) {
    return buildInstructionRow(row as ImageInstructionRow, styles);
  }
  return {
    offset: row.offset,
    height: 0,
    element: <></>,
  };
};
