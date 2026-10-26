package org.reigncarnated.menu;

import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.provider.DocumentsContract;
import java.io.*;
import java.util.*;

final class DocumentTreeSource implements DataFolderImporter.Source {
 private final ContentResolver resolver;private final Uri tree;
 DocumentTreeSource(ContentResolver resolver,Uri tree){this.resolver=resolver;this.tree=tree;}
 DataFolderImporter.Node root(){return new DataFolderImporter.Node(DocumentsContract.getTreeDocumentId(tree),"Selected folder",true,-1);}
 public List<DataFolderImporter.Node> children(DataFolderImporter.Node directory)throws IOException{
  Uri uri=DocumentsContract.buildChildDocumentsUriUsingTree(tree,directory.id);
  String[] columns={DocumentsContract.Document.COLUMN_DOCUMENT_ID,DocumentsContract.Document.COLUMN_DISPLAY_NAME,DocumentsContract.Document.COLUMN_MIME_TYPE,DocumentsContract.Document.COLUMN_SIZE};
  List<DataFolderImporter.Node> nodes=new ArrayList<>();
  try(Cursor cursor=resolver.query(uri,columns,null,null,null)){
   if(cursor==null)throw new IOException("Cannot list selected folder");
   while(cursor.moveToNext()){
    if(nodes.size()>=10000)throw new IOException("Selected directory is too large");
    String id=cursor.getString(0),name=cursor.getString(1);if(id==null||name==null)throw new IOException("File provider returned an unnamed entry");
    nodes.add(new DataFolderImporter.Node(id,name,DocumentsContract.Document.MIME_TYPE_DIR.equals(cursor.getString(2)),cursor.isNull(3)?-1:cursor.getLong(3)));
   }
  }catch(SecurityException|IllegalArgumentException e){throw new IOException("Folder permission was lost; select it again",e);}
  return nodes;
 }
 public InputStream open(DataFolderImporter.Node file)throws IOException{
  try{return resolver.openInputStream(DocumentsContract.buildDocumentUriUsingTree(tree,file.id));}catch(SecurityException e){throw new IOException("Cannot read "+file.name+"; select the folder again",e);}
 }
}
