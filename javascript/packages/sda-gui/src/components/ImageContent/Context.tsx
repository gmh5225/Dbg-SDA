import { createContext, useContext, useState } from 'react';
import { ObjectId } from 'sda-electron/api/common';
import { Jump } from 'sda-electron/api/image';
import { nullFunction } from 'utils';

export interface ImageContentFunctions {
  goToOffset: (offset: number) => Promise<boolean>;
}

export interface ImageContentView {
  showPcode: boolean;
}

export interface ImageContentContextValue {
  imageId: ObjectId;
  functions: ImageContentFunctions;
  setFunctions: (functions: ImageContentFunctions) => void;
  view: ImageContentView;
  setView: (view: ImageContentView) => void;
  selectedJump?: Jump;
  setSelectedJump?: (jump?: Jump) => void;
  rowSelection: {
    selectedRows: number[];
    setSelectedRows: (rows: number[]) => void;
    firstSelectedRow?: number;
    setFirstSelectedRow?: (row?: number) => void;
    lastSelectedRow?: number;
    setLastSelectedRow?: (row?: number) => void;
  };
}

export const ImageContentContext = createContext<ImageContentContextValue | null>(null);

export const useImageContent = () => {
  const ctx = useContext(ImageContentContext);
  if (!ctx) throw new Error('ImageContentContext is not set');
  return ctx;
};

export interface ImageContentProviderProps {
  imageId: ObjectId;
  children?: React.ReactNode;
}

export const ImageContentProvider = ({ imageId, children }: ImageContentProviderProps) => {
  const [functions, setFunctions] = useState<ImageContentFunctions>({
    goToOffset: nullFunction,
  });
  const [view, setView] = useState<ImageContentView>({
    showPcode: false,
  });
  const [selectedRows, setSelectedRows] = useState<number[]>([]);
  const [firstSelectedRow, setFirstSelectedRow] = useState<number>();
  const [lastSelectedRow, setLastSelectedRow] = useState<number>();
  const [selectedJump, setSelectedJump] = useState<Jump>();

  return (
    <ImageContentContext.Provider
      value={{
        imageId,
        functions,
        setFunctions,
        view,
        setView,
        selectedJump,
        setSelectedJump,
        rowSelection: {
          selectedRows,
          setSelectedRows,
          firstSelectedRow,
          setFirstSelectedRow,
          lastSelectedRow,
          setLastSelectedRow,
        },
      }}
    >
      {children}
    </ImageContentContext.Provider>
  );
};

export const ImageContentBridgeProvider = ImageContentContext.Provider;
export const ImageContentBridgeConsumer = ImageContentContext.Consumer;
